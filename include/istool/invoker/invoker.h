//
// Created by pro on 2022/2/15.
//

#ifndef ISTOOL_INVOKER_H
#define ISTOOL_INVOKER_H

#include "istool/basic/specification.h"
#include "istool/basic/time_guard.h"
#include "istool/basic/verifier.h"
#include "istool/solver/solver.h"
#include "glog/logging.h"

enum class SolverToken {
    EUSOLVER,
    MAXFLASH,
    POLYGEN
};

class InvokeConfig {
    class InvokeConfigItem {
    public:
        void* data;
        std::function<void(void*)> free_operator;
        InvokeConfigItem(void* _data, std::function<void(void*)> _free_operator);
        ~InvokeConfigItem();
    };
public:
    std::unordered_map<std::string, InvokeConfigItem*> item_map;

    template<class T> void set(const std::string& name, const T& w) {
        auto it = item_map.find(name);
        if (it != item_map.end()) delete it->second;
        auto free_operator = [](void* w){delete static_cast<T*>(w);};
        auto* item = new InvokeConfigItem(new T(w), free_operator);
        item_map[name] = item;
    }
    template<class T> T access(const std::string& name, const T& default_w) const {
        auto it = item_map.find(name);
        if (it == item_map.end()) return default_w;
        auto *res = static_cast<T *>(it->second->data);
        if (!res) {
            LOG(FATAL) << "Config Error: unexpected type for config " << name;
        }
        return *res;
    }
    ~InvokeConfig();
};

namespace invoker {
    namespace single {
        /**
         * @config "prepare"
         *   Type: std::function<void(Grammar*, Env* env, const IOExample&)> (VSAEnvSetter);
         *   Set constants that are used for building VSAs.
         *   Default: For String, defined in executor/invoker/vsa_invoker.cpp
         *
         * @config "model"
         *   Type: TopDownModel*
         *   A cost model for programs in the program space. MaxFlash always returns the solution with the minimum cost.
         *   Default: ext::vsa::getSizeModel()
         */
        Solver* buildMaxFlash(Specification* spec, Verifier* v, const InvokeConfig& config);
        Solver* buildPolyGen(Specification* spec, Verifier* v, const InvokeConfig& config);

        /**
         * @config "memory"
         * Type: int
         * The memory limit (GB) for external solvers
         * Default: 4
         */
        Solver* buildExternalEuSolver(Specification* spec, Verifier* v, const InvokeConfig& config);
    }

    FunctionContext synthesis(Specification* spec, Verifier* v, SolverToken solver_token, TimeGuard* guard, const InvokeConfig& config={});
    std::pair<int, FunctionContext> getExampleNum(Specification* spec, Verifier* v, SolverToken solver_token, TimeGuard* guard, const InvokeConfig& config={});
    SolverToken string2TheoryToken(const std::string& name);
}

#endif //ISTOOL_INVOKER_H

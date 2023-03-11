//
// Created by pro on 2022/2/15.
//

#include "istool/invoker/invoker.h"
#include "istool/selector/selector.h"
#include "glog/logging.h"

InvokeConfig::~InvokeConfig() {
    for (auto& item: item_map) delete item.second;
}

InvokeConfig::InvokeConfigItem::InvokeConfigItem(void *_data, std::function<void (void *)> _free_operator):
    data(_data), free_operator(_free_operator) {
}
InvokeConfig::InvokeConfigItem::~InvokeConfigItem() {
    free_operator(data);
}

#define RegisterSolverBuilder(name) solver = invoker::single::build ## name(spec, v, config); break

FunctionContext invoker::synthesis(Specification *spec, Verifier *v, SolverToken solver_token, TimeGuard* guard, const InvokeConfig& config) {
    Solver* solver = nullptr;
    switch (solver_token) {
        case SolverToken::POLYGEN:
            RegisterSolverBuilder(PolyGen);
        case SolverToken::MAXFLASH:
            RegisterSolverBuilder(MaxFlash);
        case SolverToken::EUSOLVER:
            RegisterSolverBuilder(ExternalEuSolver);
        default:
            LOG(FATAL) << "Unknown solver token";
    }
    auto res = solver->synthesis(guard);
    delete solver;
    return res;
}

std::pair<int, FunctionContext> invoker::getExampleNum(Specification *spec, Verifier *v, SolverToken solver_token, TimeGuard* guard, const InvokeConfig& config) {
    auto* s = new DirectSelector(v);
    auto res = synthesis(spec, s, solver_token, guard, config);
    int num = s->example_count;
    delete s;
    return {num, res};
}

namespace {
    std::unordered_map<std::string, SolverToken> token_map {
            {"maxflash", SolverToken::MAXFLASH},
            {"polygen", SolverToken::POLYGEN},
            {"eusolver", SolverToken::EUSOLVER}
    };
}

SolverToken invoker::string2TheoryToken(const std::string &name) {
    if (token_map.find(name) == token_map.end()) {
        LOG(FATAL) << "Unknown solver name " << name;
    }
    return token_map[name];
}
//
// Created by pro on 2022/5/3.
//

#include "istool/selector/learnsy/non_interactive_learnsy.h"
#include "istool/parser/theory/basic/clia/clia.h"
#include "istool/selector/learnsy/program_adaptor.h"
#include "glog/logging.h"
#include <iomanip>
#include "istool/basic/config.h"
#include <iostream>
#include <cassert>

namespace {
    const int KDefaultExampleNumLimit = 4;
    int _getDepth(Program* p) {
        if (p->sub_list.empty()) return 0;
        int res = 0;
        for (const auto& sub: p->sub_list) {
            res = std::max(res, _getDepth(sub.get()));
        }
        return res + 1;
    }
}

namespace {
    const int KStartDepth = 3;
}

void NonInteractiveLearnSy::initScorer() {
    LOG(INFO) << "build for depth " << current_depth;
    delete scorer;
    auto* fg = builder->getFlattenGrammar(current_depth);
    scorer = scorer_builder(env, fg);
    for (auto& example: example_list) {
        scorer->pushExample(example);
    }
}

NonInteractiveLearnSy::NonInteractiveLearnSy(Env *_env, FlattenGrammarBuilder *_builder, const LearnedScorerBuilder & _score_builder):
    g(_builder->grammar), builder(_builder), env(_env), current_depth(KStartDepth), scorer(nullptr), scorer_builder(_score_builder) {
    initScorer();
    KExampleNumLimit = theory::clia::getIntValue(*(env->getConstRef(selector::random::KExampleNumLimitName, BuildData(Int, KDefaultExampleNumLimit))));
}
NonInteractiveLearnSy::~NonInteractiveLearnSy() noexcept {
    delete scorer;
}
void NonInteractiveLearnSy::addHistoryExample(const Example &inp) {
    if (example_list.size() == KExampleNumLimit) {
        scorer->popExample();
        for (int i = 1; i < KExampleNumLimit; ++i) example_list[i - 1] = example_list[i];
        example_list[KExampleNumLimit - 1] = inp;
    } else example_list.push_back(inp);
    scorer->pushExample(inp);
}
int NonInteractiveLearnSy::getBestExampleId(const PProgram& program, const ExampleList &candidate_list, int num_limit) {
    int best_id = -1; RandomSemanticsScore best_cost = 1e9;
    auto p = selector::adaptor::programAdaptorWithLIARules(program.get(), g);
    int depth = _getDepth(p.get());
    if (depth > current_depth) {
        current_depth = depth; initScorer();
    }
#ifdef DEBUG
    assert(p);
    if (p->toString() != program->toString()) {
        // LOG(INFO) << "Program adaption: " << program->toString() << " --> " << p->toString();
        for (auto& inp: candidate_list) {
            assert(env->run(p.get(), inp) == env->run(program.get(), inp));
        }
    }
#endif
    std::vector<int> id_list(candidate_list.size());
    for (int i = 0; i < id_list.size(); ++i) id_list[i] = i;
    std::shuffle(id_list.begin(), id_list.end(), env->random_engine);
    if (id_list.size() > num_limit) id_list.resize(num_limit);
    for (auto id: id_list) {
        auto& inp = candidate_list[id];
        auto cost = scorer->getScore(p, inp);
        printf("%s %.20Lf\n", data::dataList2String(inp).c_str(), cost);
        if (cost < best_cost) {
            best_cost = cost; best_id = id;
        }
    }
    return best_id;
}

FiniteNonInteractiveLearnSy::FiniteNonInteractiveLearnSy(Specification *spec, FlattenGrammarBuilder *builder, const LearnedScorerBuilder& _scorer_builder):
        NonInteractiveLearnSy(spec->env.get(), builder, _scorer_builder) {
    io_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (!io_space) {
        LOG(FATAL) << "FiniteRandomSemanticsSelector supports only FiniteIOExampleSpace";
    }
    for (auto& example: io_space->example_space) {
        io_example_list.push_back(io_space->getIOExample(example));
    }
}
bool FiniteNonInteractiveLearnSy::verify(const FunctionContext &info, Example *counter_example) {
    auto p = info.begin()->second;
    std::vector<int> counter_id_list;
    ExampleList candidate_inp_list;
    for (int i = 0; i < io_example_list.size(); ++i) {
        auto& example = io_example_list[i];
        if (!(env->run(p.get(), example.first) == example.second)) {
            counter_id_list.push_back(i);
            candidate_inp_list.push_back(example.first);
            if (!counter_example) return false;
        }
    }
    if (candidate_inp_list.empty()) return true;
    int best_id = counter_id_list[getBestExampleId(p, candidate_inp_list)];
    *counter_example = io_space->example_space[best_id];
    addHistoryExample(io_example_list[best_id].first);
    return false;
}

Z3RandomSemanticsSelector::Z3RandomSemanticsSelector(Specification *spec, FlattenGrammarBuilder *builder, const LearnedScorerBuilder& _scorer_builder, int _KExampleNum):
        Z3Verifier(dynamic_cast<Z3ExampleSpace*>(spec->example_space.get())), NonInteractiveLearnSy(spec->env.get(), builder, _scorer_builder),
        num_limit(_KExampleNum) {
    z3_io_space = dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get());
    if (!z3_io_space) {
        LOG(FATAL) << "Z3RandomSemanticsSelector supports only Z3IOExampleSpace";
    }
}
bool Z3RandomSemanticsSelector::verify(const FunctionContext &info, Example *counter_example) {
    auto p = info.begin()->second;
    z3::solver s(ext->ctx);
    prepareZ3Solver(s, info);
    auto res = s.check();
    if (res == z3::unsat) return true;
    if (res != z3::sat) {
        LOG(FATAL) << "Z3 failed with " << res;
    }
    ExampleList example_list;
    Example example;
    auto model = s.get_model();
    getExample(model, &example);
    example_list.push_back(example);
    auto param_list = getParamVector();
    for (int _ = 1; _ < num_limit; ++_) {
        z3::expr_vector block(ext->ctx);
        for (int i = 0; i < param_list.size(); ++i) {
            block.push_back(ext->buildConst(example[i]) != param_list[i]);
        }
        s.add(z3::mk_or(block));
        auto status = s.check();
        if (status == z3::unsat) break;
        model = s.get_model(); getExample(model, &example);
        example_list.push_back(example);
    }
    ExampleList inp_list;
    for (auto& current_example: example_list) {
        inp_list.push_back(z3_io_space->getInput(current_example));
    }
    int best_id = getBestExampleId(p, inp_list);
    addHistoryExample(inp_list[best_id]);
    *counter_example = example_list[best_id];
    return false;
}

const std::string selector::random::KExampleNumLimitName = "RandomSelector@ExampleLimit";
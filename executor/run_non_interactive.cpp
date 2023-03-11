//
// Created by pro on 2022/2/15.
//

#include "istool/basic/config.h"
#include "istool/selector/split/finite_splitor.h"
#include "istool/invoker/invoker.h"
#include "istool/parser/sygus.h"
#include "istool/parser/parser/parser.h"
#include "istool/ext/z3/z3_example_space.h"
#include "istool/selector/baseline/biased_bitvector_selector.h"
#include "istool/parser/theory/basic/clia/clia.h"
#include "istool/parser/samplesy_dsl.h"
#include "istool/ext/vsa/vsa_extension.h"
#include "istool/parser/theory/basic/string/str.h"
#include "istool/parser/theory/witness/clia/clia_witness.h"
#include "istool/parser/theory/witness/string/string_witness.h"
#include "istool/selector/learnsy/non_interactive_learnsy.h"
#include "istool/selector/learnsy/learned_scorer.h"
#include "istool/selector/baseline/significant_input.h"
#include <gflags/gflags.h>
#include <iostream>
#include <istool/ext/vsa/vsa_inside_checker.h>

auto KStringPrepare = [](Grammar* g, Env* env, const IOExample& io_example) {
    DataList string_const_list, string_input_list;
    std::unordered_set<std::string> const_set;
    for (auto* symbol: g->symbol_list) {
        for (auto* rule: symbol->rule_list) {
            auto* sem = grammar::getConstSemantics(rule);
            if (sem) {
                auto* sv = dynamic_cast<StringValue*>(sem->w.get());
                if (!sv) continue;
                if (const_set.find(sv->s) == const_set.end()) {
                    const_set.insert(sv->s);
                    string_const_list.push_back(sem->w);
                }
            }
        }
    }
    for (const auto& inp: io_example.first) {
        auto* sv = dynamic_cast<StringValue*>(inp.get());
        if (sv) string_input_list.push_back(inp);
    }

    int int_max = 1;
    for (const auto& s: string_const_list) {
        int_max = std::max(int_max, int(theory::string::getStringValue(s).length()));
    }
    for (const auto& s: string_input_list) {
        int_max = std::max(int_max, int(theory::string::getStringValue(s).length()));
    }
    for (const auto& inp: io_example.first) {
        auto* iv = dynamic_cast<IntValue*>(inp.get());
        if (iv) int_max = std::max(int_max, iv->w);
    }

    env->setConst(theory::clia::KWitnessIntMinName, BuildData(Int, -int_max));
    env->setConst(theory::string::KStringConstList, string_const_list);
    env->setConst(theory::string::KStringInputList, string_input_list);
    env->setConst(theory::clia::KWitnessIntMaxName, BuildData(Int, int_max));
};

int KExampleNum = 5;

Verifier* _buildRandomsSemanticsVerifier(Specification* spec, int num, const std::string& name, TopDownModel* model) {
    FlattenGrammarBuilder* builder;
    auto* g = spec->info_list[0]->grammar;

    if (name == "maxflash") {
        auto* vsa_ext = ext::vsa::getExtension(spec->env.get());
        vsa_ext->setEnvSetter(KStringPrepare);
        auto* example_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
        IOExampleList io_list;
        for (auto& example: example_space->example_space) {
            io_list.push_back(example_space->getIOExample(example));
        }
        auto tool = std::make_shared<FiniteEquivalenceCheckerTool>(spec->env.get(), example_space);
        auto* validator = new AllValidProgramChecker();
        builder = new MergedFlattenGrammarBuilder(g, model, spec->env.get(), num, validator, tool);
    } else {
        builder = new TrivialFlattenGrammarBuilder(g, model, spec->env.get(), num, new AllValidProgramChecker());
    }
    LearnedScorerBuilder scorer_builder = [](Env* env, FlattenGrammar* fg)->Scorer* {
        auto* holder = new BasicSampleStructureHolder(env, fg);
        auto* learner = new FixedSampleUnifiedEquivalenceModelLearner(holder);
        return new OptimizedScorer(env, fg, learner);
    };
    if (dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get())) {
        return new Z3RandomSemanticsSelector(spec, builder, scorer_builder, KExampleNum);
    } else {
        return new FiniteNonInteractiveLearnSy(spec, builder, scorer_builder);
    }
}

const std::string KBVBenchmarkHead = "(define-fun ehad ((x (BitVec 64))) (BitVec 64)"
                                     "(bvlshr x #x0000000000000001))"
                                     "(define-fun arba ((x (BitVec 64))) (BitVec 64)"
                                     "(bvlshr x #x0000000000000004))"
                                     "(define-fun shesh ((x (BitVec 64))) (BitVec 64)"
                                     "(bvlshr x #x0000000000000010))"
                                     "(define-fun smol ((x (BitVec 64))) (BitVec 64)"
                                     "(bvshl x #x0000000000000001))"
                                     "(define-fun if0 ((x (BitVec 64)) (y (BitVec 64)) (z (BitVec 64))) (BitVec 64)"
                                     "(ite (= x #x0000000000000001) y z));\n";

std::string getTaskName(const std::string& task_file) {
    int pos = task_file.length() - 1;
    while (task_file[pos] != '/') --pos;
    auto name = task_file.substr(pos + 1);
    if (name.length() > 3 && name.substr(name.length() - 3) == ".sl") {
        name = name.substr(0, name.length() - 3);
    }
    return name;
}

DEFINE_string(benchmark, "", "The absolute path of the benchmark file (.sl)");
DEFINE_string(output, "", "The absolute path of the output file");
DEFINE_string(solver, "", "The name of the PBE solver (eusolver/maxflash/polygen)");
DEFINE_string(selector, "learnsy", "The name of the question selector (learnsy/default/biased/significant)");
DEFINE_string(model, "", "The absolute path of a file decribing the PRTG required by leanrsy. " \
    "A trivial PRTG is used by default, where ruls expanding from the same non-terminal are assigned with the same porbability.");
DEFINE_int32(flatten, 100, "The number limit of rules introduced by flattening. The default limit is set to 100.");
DEFINE_int32(seed, 0, "Random Seed");

int main(int argc, char** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    auto benchmark_path = FLAGS_benchmark, output_path = FLAGS_output, solver_name = FLAGS_solver, selector_name = FLAGS_selector;
    auto* guard = new TimeGuard(1e9);

    if (selector_name == "significant") parser::KIsRemoveDuplicated = false;
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_path);
    spec->env->setRandomSeed(FLAGS_seed);

    if (sygus::getSyGuSTheory(spec->env.get()) == TheoryToken::BV) {
        sygus::setSyGuSHeader(spec->env.get(), KBVBenchmarkHead);
    }
    auto* v = sygus::getVerifier(spec);

    if (selector_name == "default") ;
    else if (selector_name == "significant")
        v = new SignificantInputSelector(dynamic_cast<FiniteExampleSpace*>(spec->example_space.get()), getTaskName(benchmark_path));
    else if (selector_name == "biased")
        v = new BiasedBitVectorSelector(dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get()));
    else {
        auto* model = ext::vsa::getSizeModel();
        if (!FLAGS_model.empty()) model = ext::vsa::loadNFoldModel(FLAGS_model, getTaskName(benchmark_path));
        v = _buildRandomsSemanticsVerifier(spec, FLAGS_flatten, solver_name, model);
    }
    auto solver_token = invoker::string2TheoryToken(solver_name);
    InvokeConfig config;
    auto result = invoker::getExampleNum(spec, v, solver_token, guard);
    std::cout << result.first << " " << result.second.toString() << std::endl;
    std::cout << guard->getPeriod() << std::endl;
    if (!output_path.empty()) {
        FILE *f = fopen(output_path.c_str(), "w");
        fprintf(f, "%d %s\n", result.first, result.second.toString().c_str());
        fprintf(f, "%.10lf\n", guard->getPeriod());
    }
}
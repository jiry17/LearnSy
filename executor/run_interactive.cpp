//
// Created by pro on 2022/5/8.
//

#include <cassert>
#include "istool/invoker/invoker_util.h"
#include "istool/basic/specification.h"
#include "istool/invoker/invoker.h"
#include "istool/selector/samplesy/samplesy.h"
#include "istool/selector/split/finite_splitor.h"
#include "istool/selector/split/z3_splitor.h"
#include "istool/solver/vsa/vsa_builder.h"
#include "istool/parser/theory/basic/clia/clia.h"
#include "istool/parser/theory/witness/string/string_witness.h"
#include "istool/selector/baseline/random_selector.h"
#include "istool/basic/config.h"
#include "istool/selector/samplesy/finite_equivalence_checker.h"
#include "istool/selector/samplesy/vsa_seed_generator.h"
#include "istool/selector/samplesy/different_program_generator.h"
#include "istool/selector/samplesy/z3_equivalence_checker.h"
#include "istool/selector/learnsy/learned_scorer.h"
#include "istool/selector/learnsy/interactive_learnsy.h"
#include "istool/parser/theory/basic/clia/clia_example_sampler.h"
#include "istool/ext/vsa/vsa_inside_checker.h"
#include <iostream>
#include <gflags/gflags.h>

typedef std::pair<int, FunctionContext> SynthesisResult;

Splitor* getSplitor(Specification* spec) {
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (fio) return new FiniteSplitor(fio);
    auto* zio = dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get());
    if (zio) {
        auto sig = zio->sig_map.begin()->second;
        return new Z3Splitor(spec->example_space.get(), sig.second, sig.first);
    }
    LOG(FATAL) << "Unsupported type of ExampleSpace for the Splitor";
}

GrammarEquivalenceChecker* getEquivalenceChecker(Specification* spec, const PVSABuilder& builder) {
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (fio) {
        auto* diff_gen = new VSADifferentProgramGenerator(builder);
        return new FiniteGrammarEquivalenceChecker(diff_gen, fio);
    }
    auto* zio = dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get());
    if (zio) {
        auto* ext = ext::z3::getExtension(spec->env.get());
        auto* g = spec->info_list[0]->grammar;
        auto* env = spec->env.get();
        auto range_limit = invoker::getDefaultIntRangeCons(spec->info_list[0].get(), env);
        return new Z3GrammarEquivalenceChecker(g, ext, spec->info_list[0]->inp_type_list, range_limit);
    }
    LOG(FATAL) << "Unsupported type of ExampleSpace for the EquivalenceChecker";
}

SeedGenerator* getSeedGenerator(Specification* spec, const PVSABuilder& builder) {
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (fio) {
        auto* diff_gen = new VSADifferentProgramGenerator(builder);
        return new FiniteVSASeedGenerator(builder, new VSASizeBasedSampler(spec->env.get()), diff_gen, fio);
    }
    auto* zio = dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get());
    if (zio) {
        return new Z3VSASeedGenerator(spec, builder, new VSASizeBasedSampler(spec->env.get()));
    }
    LOG(FATAL) << "Unsupported type of ExampleSpace for the SeedGenerator";
}

SynthesisResult invokeSampleSy(Specification* spec, TimeGuard* guard) {
    auto* pruner = new TrivialPruner();
    auto& info = spec->info_list[0];
    auto builder = std::make_shared<DFSVSABuilder>(info->grammar, pruner, spec->env.get());

    auto* splitor = getSplitor(spec);
    auto* checker = getEquivalenceChecker(spec, builder);
    auto* gen = getSeedGenerator(spec, builder);
    auto* solver = new SampleSy(spec, splitor, gen, checker);
    auto res = solver->synthesis(guard);
    return {solver->example_count, res};
}

SynthesisResult invokeRandomSy(Specification* spec, TimeGuard* guard) {
    auto* pruner = new TrivialPruner();
    auto& info = spec->info_list[0];

    auto builder = std::make_shared<DFSVSABuilder>(info->grammar, pruner, spec->env.get());
    auto* checker = getEquivalenceChecker(spec, builder);

    auto* example_space = spec->example_space.get();
    if (dynamic_cast<FiniteIOExampleSpace*>(example_space)) {
        auto* diff_gen = new VSADifferentProgramGenerator(builder);
        auto *solver = new FiniteRandomSelector(spec, checker, diff_gen);
        auto res = solver->synthesis(nullptr);
        return {solver->example_count, res};
    }
    auto* zio = dynamic_cast<Z3IOExampleSpace*>(example_space);
    if (zio) {
        spec->env->setConst(theory::clia::KSampleIntMinName, BuildData(Int, -invoker::getDefaultIntMax()));
        spec->env->setConst(theory::clia::KSampleIntMaxName, BuildData(Int, invoker::getDefaultIntMax()));
        auto* gen = new IntExampleGenerator(spec->env.get(), zio->type_list);
        auto* z3_verifier = new Z3Verifier(zio);
        auto* solver = new Z3RandomSelector(spec, checker, z3_verifier, gen);
        auto res = solver->synthesis(nullptr);
        return {solver->example_count, res};
    }
    assert(0);
}

FlattenGrammar* getFlattenGrammar(Specification* spec, TopDownContextGraph* graph, int num) {
    auto theory = sygus::getSyGuSTheory(spec->env.get());
    if (theory == TheoryToken::STRING) {
        auto* example_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
        example_space->removeDuplicate();
        IOExampleList io_list;
        for (auto& example: example_space->example_space) {
            io_list.push_back(example_space->getIOExample(example));
        }
        auto* g = spec->info_list[0]->grammar;
        auto* validator = new ProgramInsideVSAChecker(spec->env.get(), g, spec->example_space.get());
        auto tool = std::make_shared<FiniteEquivalenceCheckerTool>(spec->env.get(), example_space);
        return new MergedFlattenGrammar(graph, spec->env.get(), num, validator, tool);
    } else {
        return new TrivialFlattenGrammar(graph, spec->env.get(), num, new AllValidProgramChecker());
    }
}

std::string getName(FlattenGrammar* fg, const TopDownContextGraph::Edge& edge) {
    auto* sem = edge.semantics.get(); auto* ps = dynamic_cast<ParamSemantics*>(sem);
    if (ps) return fg->param_info_list[ps->id].program->toString();
    return sem->getName();
}

SynthesisResult invokeRandomSemanticsSelector(Specification* spec, int num, TimeGuard* guard, TopDownModel* model) {
    auto* pruner = new TrivialPruner();
    auto& info = spec->info_list[0];
    auto* vsa_ext = ext::vsa::getExtension(spec->env.get());
    auto theory = sygus::getSyGuSTheory(spec->env.get());
    vsa_ext->setEnvSetter(invoker::getDefaultVSAEnvSetter(theory));
    auto builder = std::make_shared<DFSVSABuilder>(info->grammar, pruner, spec->env.get());
    auto* checker = getEquivalenceChecker(spec, builder);

    auto* graph = new TopDownContextGraph(info->grammar, model, ProbModelType::NORMAL_PROB);
    auto* fg = getFlattenGrammar(spec, graph, num);

    Scorer* scorer;
    if (sygus::getSyGuSTheory(spec->env.get()) == TheoryToken::STRING) {
        auto* holder = new VSASampleStructureHolder(spec, fg, invoker::getDefaultVSAEnvSetter(theory));
        auto* learner = new FixedSampleUnifiedEquivalenceModelLearner(holder);
        scorer = new OptimizedScorer(spec->env.get(), fg, learner);
    } else {
        auto* holder = new BasicSampleStructureHolder(spec->env.get(), fg);
        auto* learner = new FixedSampleUnifiedEquivalenceModelLearner(holder);
        scorer = new OptimizedScorer(spec->env.get(), fg, learner);
    }

    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (fio) {
        auto* diff_gen = new VSADifferentProgramGenerator(builder);
        auto *solver = new FiniteInteractiveLearnSy(spec, checker, scorer, diff_gen, 50);
        auto res = solver->synthesis(guard);
        return {solver->example_count, res};
    }

    auto* zio = dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get());
    if (zio) {
        auto range_cons = invoker::getDefaultIntRangeCons(spec->info_list[0].get(), spec->env.get());
        auto* solver = new Z3InteractiveLearnSy(spec, checker, scorer, range_cons.get(), 10);
        auto res = solver->synthesis(guard);
        return {solver->example_count, res};
    }
    LOG(FATAL) << "Unsupported example space";
}

std::string getTaskName(const std::string& task_file) {
    int pos = task_file.length() - 1;
    while (task_file[pos] != '/') --pos;
    auto name = task_file.substr(pos + 1);
    if (name.length() > 3 && name.substr(name.length() - 3) == ".sl") {
        name = name.substr(0, name.length() - 3);
    }
    return name;
}

DEFINE_string(benchmark, "/home/jiry/2022A/LearnSy/tests/repair/t1.sl", "The absolute path of the benchmark file (.sl)");
DEFINE_string(output, "", "The absolute path of the output file");
DEFINE_string(selector, "learnsy", "The name of the question selector (learnsy/samplesy/randomsy)");
DEFINE_int32(sample_time, 120, "The time limit (seconds) for samplesy to perform sampling");
DEFINE_string(model, "/home/jiry/2022A/LmernSy/runner/model/intsy_repair", "The absolute path of a file decribing the PRTG required by leanrsy. " \
    "A trivial PRTG is used by default, where ruls expanding from the same non-terminal are assigned with the same porbability.");
DEFINE_int32(flatten, 3000, "The number limit of rules introduced by flattening. The default limit is set to 3000.");
DEFINE_int32(seed, 0, "Random Seed");

int main(int argc, char** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    auto benchmark_path = FLAGS_benchmark, output_path = FLAGS_output, selector_name = FLAGS_selector;
    SynthesisResult result;
    auto* spec = invoker::getSampleSyStyledTask(benchmark_path);
    auto* guard = new TimeGuard(1e9);
    spec->env->setRandomSeed(FLAGS_seed);

    auto vsa_setter = invoker::getDefaultVSAEnvSetter(sygus::getSyGuSTheory(spec->env.get()));
    auto* vsa_ext = ext::vsa::getExtension(spec->env.get());
    vsa_ext->setEnvSetter(vsa_setter);

    selector::setSplitorTimeOut(spec->env.get(), 2);
    if (selector_name == "samplesy") {
        spec->env->setConst(selector::samplesy::KSampleTimeOutLimit, BuildData(Int, FLAGS_sample_time * 1000));
        result = invokeSampleSy(spec, guard);
    } else if (selector_name == "randomsy") {
        result = invokeRandomSy(spec, guard);
    } else {
        TopDownModel* model;
        if (FLAGS_model.empty()) model = ext::vsa::getSizeModel(); else {
            auto task_name = getTaskName(benchmark_path);
            model = ext::vsa::loadNFoldModel(FLAGS_model, task_name);
        }
        result = invokeRandomSemanticsSelector(spec, FLAGS_flatten, guard, model);
    }

    std::cout << result.first << " " << result.second.toString() << std::endl;
    std::cout << guard->getPeriod() << std::endl;
    if (!output_path.empty()) {
        FILE *f = fopen(output_path.c_str(), "w");
        fprintf(f, "%d %s\n", result.first, result.second.toString().c_str());
        fprintf(f, "%.10lf\n", guard->getPeriod());
    }
}
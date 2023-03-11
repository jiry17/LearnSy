//
// Created by pro on 2022/12/27.
//

#include <istool/parser/samplesy_dsl.h>
#include <istool/parser/theory/basic/clia/clia_value.h>
#include <istool/parser/theory/witness/clia/clia_witness.h>
#include "istool/invoker/invoker_util.h"
#include <unordered_set>
#include <istool/parser/theory/basic/string/string_value.h>
#include <istool/parser/theory/witness/string/string_witness.h>
#include <istool/parser/samplesy/samplesy_witness.h>
#include <istool/parser/theory/witness/theory_witness.h>
#include <istool/parser/theory/basic/theory_semantics.h>
#include <istool/parser/samplesy/samplesy_semantics.h>
#include "glog/logging.h"
#include "istool/parser/parser/parser.h"

int invoker::getDefaultIntMax() {
    return 5;
}

namespace {
    const auto KIntPrepare = [](Grammar* g, Env* env, const IOExample& io_example) {
        int tmp_int_limit = invoker::getDefaultIntMax();
        auto* ov = dynamic_cast<IntValue*>(io_example.second.get());
        if (ov) tmp_int_limit = std::max(tmp_int_limit, ov->w);
        for (auto& data: io_example.first) {
            auto* iv = dynamic_cast<IntValue*>(data.get());
            if (iv) tmp_int_limit = std::max(tmp_int_limit, std::abs(iv->w));
        }
        env->setConst(theory::clia::KWitnessIntMinName, BuildData(Int, -tmp_int_limit * 2));
        env->setConst(theory::clia::KWitnessIntMaxName, BuildData(Int, tmp_int_limit * 2));
    };
    const auto KStringPrepare = [](Grammar* g, Env* env, const IOExample& io_example) {
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
}

VSAEnvSetter invoker::getDefaultVSAEnvSetter(TheoryToken theory) {
    if (theory == TheoryToken::CLIA) return KIntPrepare;
    if (theory == TheoryToken::STRING) return KStringPrepare;
    LOG(FATAL) << "The default VSAEnvSetter for theory " << sygus::theoryToken2String(theory) << " is unknown";
}

namespace {
    PProgram _buildIntRangeCons(const PProgram& p, Env* env) {
        auto upper_bound = program::buildConst(BuildData(Int, invoker::getDefaultIntMax()));
        auto lower_bound = program::buildConst(BuildData(Int, -invoker::getDefaultIntMax()));
        auto upper_cons = std::make_shared<Program>(env->getSemantics(">="), (ProgramList){upper_bound, p});
        auto lower_cons = std::make_shared<Program>(env->getSemantics(">="), (ProgramList){p, lower_bound});
        return std::make_shared<Program>(env->getSemantics("&&"), (ProgramList){upper_cons, lower_cons});
    }
}

PProgram invoker::getDefaultIntRangeCons(SynthInfo *info, Env *env) {
    TypeList type_list = info->inp_type_list;
    ProgramList cons_list;
    for (int i = 0; i < type_list.size(); ++i) {
        if (dynamic_cast<TInt*>(type_list[i].get())) {
            auto p = program::buildParam(i, type_list[i]);
            cons_list.push_back(_buildIntRangeCons(p, env));
        }
    }
    if (cons_list.empty()) return program::buildConst(BuildData(Bool, true));
    auto res = cons_list[0];
    for (int i = 1; i < cons_list.size(); ++i) {
        res = std::make_shared<Program>(env->getSemantics("&&"), (ProgramList){res, cons_list[i]});
    }
    return res;
}

namespace {
    std::string _getTaskName(const std::string& path) {
        int last = 0;
        for (auto i = path.find("/"); i != std::string::npos; i = path.find("/", i + 1)) last = i;
        return path.substr(last + 1);
    }

    // The depth is the same as the evaluation in PLDI20 "Question Selection for Interactive Program Synthesis"
    const std::unordered_map<std::string, int> _KSpecialDepth = {
            {"t8.sl", 2}, {"t10.sl", 3}, {"t17.sl", 3}, {"t20.sl", 3}, {"t3.sl", 3}, {"t7.sl", 3},
            {"dr-name.sl", 7}, {"dr-name-long.sl", 7}, {"dr-name-long-repeat.sl", 7}, {"dr-name_small.sl", 7},
            {"2171308.sl", 7}, {"exceljet1.sl", 7}, {"get-domain-name-from-url.sl", 7}, {"get-last-name-from-name-with-comma.sl", 8},
            {"initials.sl", 8}, {"initials-long.sl", 8}, {"initials-long-repeat.sl", 8}, {"initials_small.sl", 8},
            {"stackoverflow10.sl", 8}, {"stackoverflow11.sl", 7}
    };
    int _getGrammarDepth(Specification* spec, const std::string& task_name) {
        auto it = _KSpecialDepth.find(task_name);
        if (it != _KSpecialDepth.end()) {
            return it->second;
        }
        auto theory = sygus::getSyGuSTheory(spec->env.get());
        if (theory == TheoryToken::STRING) return 6;
        else if (theory == TheoryToken::CLIA) return 4;
        LOG(FATAL) << "GrammarDepth undefined for the current theory";
    }

    void _collectIntParam(const PProgram& p, std::unordered_map<int, PProgram>& res) {
        auto* ps = dynamic_cast<ParamSemantics*>(p->semantics.get());
        if (ps && dynamic_cast<TInt*>(ps->oup_type.get())) res[ps->id] = p;
        for (auto& sub: p->sub_list) {
            _collectIntParam(sub, res);
        }
    }

    void _registerSampleSyBasic(Env *env) {
        sygus::setTheory(env, TheoryToken::STRING);
        sygus::loadSyGuSTheories(env, theory::loadBasicSemantics);
        env->setSemantics("", std::make_shared<DirectSemantics>());
        env->setSemantics("replace", std::make_shared<samplesy::StringReplaceAllSemantics>());
        env->setSemantics("delete", std::make_shared<samplesy::StringDeleteSemantics>());
        env->setSemantics("substr", std::make_shared<samplesy::StringAbsSubstrSemantics>());
        env->setSemantics("indexof", std::make_shared<samplesy::StringIndexOfSemantics>());
        env->setSemantics("move", std::make_shared<samplesy::IndexMoveSemantics>());
    }

    void _registerSampleSyWitness(Env* env) {
        sygus::loadSyGuSTheories(env, theory::loadWitnessFunction);
        auto* const_list = env->getConstListRef(theory::string::KStringConstList);
        auto* input_list = env->getConstListRef(theory::string::KStringInputList);
        auto* int_max = env->getConstRef(theory::clia::KWitnessIntMaxName);
        auto* ext = ext::vsa::getExtension(env);

        ext->registerWitnessFunction("replace", new samplesy::StringReplaceAllWitnessFunction(const_list, input_list));
        ext->registerWitnessFunction("delete", new samplesy::StringDeleteWitnessFunction(const_list, input_list));
        ext->registerWitnessFunction("substr", new samplesy::StringAbsSubstrWitnessFunction(input_list));
        ext->registerWitnessFunction("indexof", new samplesy::StringIndexOfWitnessFunction(input_list));
        ext->registerWitnessFunction("move", new samplesy::IndexMoveWitnessFunction(int_max));
    }
}

Specification * invoker::getSampleSyStyledTask(const std::string &path) {
    auto *spec = parser::getSyGuSSpecFromFile(path);
    env::setTimeSeed(spec->env.get());
    std::string task_name = _getTaskName(path);

    int depth = _getGrammarDepth(spec, task_name);
    auto& info = spec->info_list[0];
    auto theory = sygus::getSyGuSTheory(spec->env.get());
    if (theory == TheoryToken::STRING) {
        _registerSampleSyBasic(spec->env.get());
        _registerSampleSyWitness(spec->env.get());
        info->grammar = samplesy::rewriteGrammar(info->grammar, spec->env.get(),
                                                 dynamic_cast<FiniteIOExampleSpace *>(spec->example_space.get()));
    } else if (theory == TheoryToken::CLIA) {
        theory::clia::loadWitnessFunction(spec->env.get());
        auto* example_space = dynamic_cast<ExampleSpace*>(spec->example_space.get());
        std::unordered_map<int, PProgram> int_params;
        _collectIntParam(example_space->cons_program, int_params);
        for (auto& info: int_params) {
            auto range_cons = _buildIntRangeCons(info.second, spec->env.get());
            example_space->cons_program = std::make_shared<Program>(spec->env->getSemantics("=>"),
                                                                    (ProgramList){range_cons, example_space->cons_program});
        }
    }
    info->grammar = grammar::generateHeightLimitedGrammar(info->grammar, depth);
    return spec;
}
//
// Created by pro on 2021/12/31.
//

#include "istool/parser/theory/witness/theory_witness.h"
#include "istool/parser/theory/witness/clia/clia_witness.h"
#include "istool/parser/theory/witness/string/string_witness.h"
#include "istool/parser/theory/theory.h"
#include "glog/logging.h"

void theory::loadWitnessFunction(Env *env, TheoryToken token) {
    LOG(INFO) << sygus::theoryToken2String(token);
    switch (token) {
        case TheoryToken::CLIA: {
            theory::clia::loadWitnessFunction(env);
            return;
        }
        case TheoryToken::STRING: {
            theory::string::loadWitnessFunction(env);
            return;
        }
        // case TheoryToken::BV:
        default:
            LOG(FATAL) << "Unsupported theory";
    }
}
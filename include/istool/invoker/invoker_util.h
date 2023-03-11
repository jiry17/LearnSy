//
// Created by pro on 2022/12/27.
//

#ifndef ISTOOL_INVOKER_UTIL_H
#define ISTOOL_INVOKER_UTIL_H

#include "istool/parser/theory/theory.h"
#include "istool/ext/vsa/vsa_extension.h"

namespace invoker {
    int getDefaultIntMax();
    VSAEnvSetter getDefaultVSAEnvSetter(TheoryToken theory);
    PProgram getDefaultIntRangeCons(SynthInfo* info, Env* env);
    Specification* getSampleSyStyledTask(const std::string& name);
}

#endif //ISTOOL_INVOKER_UTIL_H

//
// Created by pro on 2022/2/2.
//

#ifndef ISTOOL_SAMPLESY_DSL_H
#define ISTOOL_SAMPLESY_DSL_H

#include <istool/basic/specification.h>
#include "istool/basic/grammar.h"
#include "istool/basic/example_space.h"

namespace samplesy {
    Grammar* getSampleSyGrammar(const TypeList& inp_types, const PType& oup, const DataList& const_list, Env* env);
    Grammar* rewriteGrammar(Grammar* g, Env* env, FiniteIOExampleSpace* io_space);
}

#endif //ISTOOL_SAMPLESY_DSL_H

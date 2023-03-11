//
// Created by pro on 2022/5/5.
//

#ifndef ISTOOL_PROGRAM_ADAPTOR_H
#define ISTOOL_PROGRAM_ADAPTOR_H

#include "istool/basic/grammar.h"

namespace selector::adaptor {
    /**
     * @breif: Adapt the synthesized program into the grammar.
     * PolyGen may transform the grammar via LIA axioms and thus may return a program outside the original grammar.
     * For example, given grammar S -> S + S | Param0, PolyGen may synthesize 2 * Param0 as the result.
     * This function fits the synthesized program into the grammar using LIS axioms.
     */
    PProgram programAdaptorWithLIARules(Program* p, Grammar* g);
}

#endif //ISTOOL_PROGRAM_ADAPTOR_H

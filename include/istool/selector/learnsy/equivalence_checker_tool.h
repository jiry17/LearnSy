//
// Created by pro on 2022/5/19.
//

#ifndef ISTOOL_EQUIVALENCE_CHECKER_TOOL_H
#define ISTOOL_EQUIVALENCE_CHECKER_TOOL_H

#include "istool/basic/program.h"
#include "istool/basic/example_sampler.h"
#include "istool/basic/example_space.h"
#include "istool/ext/z3/z3_extension.h"

/**
 * @brief: The base class for supporting queries for the semantical equivalence among a set of programs.
 * This class maintains a set of programs and supports two kinds of queries.
 * 1. Given a program, check whether there is a semantically equivalent program inside the set.
 * 2. Given a program, check whether it always outputs the same constant.
 * This class is used to merge equivalent rules while performing flattening.
 *
 * @method queryProgram(p):
 * Check whether there is a program in the set that is semantically equivalent to p. Return this program when such a
 * program exists, and return nullptr otherwise.
 *
 * @method insertProgram(p):
 * On the basis of queryProgram(), return program p if there is no program semantically equivalent to p.
 *
 * @method getConst(p):
 * Check whether program p always outputs the same constant. Return this constant when such a constant exists, and
 * return Null otherwise.
 */
class EquivalenceCheckTool {
public:
    virtual PProgram queryProgram(const PProgram& p) = 0;
    virtual PProgram insertProgram(const PProgram& p) = 0;
    virtual Data getConst(Program* p) = 0;
};

typedef std::shared_ptr<EquivalenceCheckTool> PEquivalenceCheckTool;


/**
 * @brief: A direct implementation of EquivalenceCheckTool when the input space is specified by a list of examples.
 * Define the feature of a program as its outputs on all available examples. Then two programs are semantically
 * equivalent if and only if their features are the same.
 *
 * @field feature_map records th feature for each program in the set.
 *
 * @method getFeature(p):
 * Calculate the feature of a program.
 *
 * @method queryProgram(p):
 * The check is performed by calculating the feature of p and then querying feature_map with the feature.
 *
 * @method getConst(p):
 * The check is performed by enumerating all available inputs and then checking whether the outputs are the same.
 */
class FiniteEquivalenceCheckerTool: public EquivalenceCheckTool {
public:
    DataStorage inp_pool;
    Env* env;
    std::unordered_map<std::string, PProgram> feature_map;
    std::string getFeature(Program* p);
    FiniteEquivalenceCheckerTool(Env* _env, FiniteIOExampleSpace* fio);
    virtual PProgram insertProgram(const PProgram& p);
    virtual PProgram queryProgram(const PProgram& p);
    virtual Data getConst(Program* p);
    virtual ~FiniteEquivalenceCheckerTool() = default;
};

#endif //ISTOOL_EQUIVALENCE_CHECKER_TOOL_H

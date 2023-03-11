//
// Created by pro on 2022/5/12.
//

#ifndef ISTOOL_INTERACTIVE_LEARNSY_H
#define ISTOOL_INTERACTIVE_LEARNSY_H

#include "istool/selector/selector.h"
#include "istool/selector/learnsy/unified_equivalence_model.h"
#include "istool/selector/learnsy/learned_scorer.h"
#include "istool/ext/z3/z3_verifier.h"
#include "istool/selector/samplesy/different_program_generator.h"

/**
 * @breif: The base class of LearnSy for interactive tasks.
 * This class includes some utilities to implement LearnSy for interactive tasks.
 *
 * @field scorer evaluates the approximated objective value for a candidate input.
 * @field example_num records the number of examples considered by scorer. To limit the time cost of the question
 * selection, LearnSy considers up to KExampleNumLimit examples while evaluating the approximated objective value.
 *
 * @method: getBestExampleId(program, candidate_list, num_limit)
 * Select the input with the best approximated objective value among candidate_list when the program synthesized by
 * the PBE solver is program.
 * To limit the time cost of the question selection, LearnSy considers only up to num_limit inputs in each selection.
 * When the size of candidate_list is larger than this limit, num_limit examples will be selected randomly.
 *
 * @method addHistoryExample(inp):
 * Take a new example into consideration. This method receives only the input since the output will not be used.
 * When there are already example_num examples, the one that is selected earliest will be ignored.
 */
class InteractiveLearnSy {
public:
    Env* env;
    Grammar* g;
    Scorer* scorer;
    int example_num, KExampleNumLimit;
    InteractiveLearnSy(Env* _env, Grammar* _g, Scorer* _scorer);
    int getBestExampleId(const PProgram& program, const ExampleList& candidate_list, int num_limit);
    void addHistoryExample(const Example& inp);
    ~InteractiveLearnSy();
};

/**
 * @brief: LearnSy for interactive tasks where the input space is described by a set of input-output examples.
 * Field io_example_list stores all input-output examples available in the synthesis task.
 * Given an input, field g can generate two remaining programs that output differently on the input or report that
 * all remaining programs output the same on the given input. LearnSy uses this field to check whether an input can
 * make progress for interactive synthesis, i.e., can exclude some remaining programs.
 *
 * @method getNextExample(x, y)
 * In each iteration, LearnSy filters out those inputs that can make progress for interactive synthesis (by invoking
 * g) and then selects the one with the best approximated objective value among them.
 */
class FiniteInteractiveLearnSy: public CompleteSelector, public InteractiveLearnSy {
public:
    FiniteIOExampleSpace* fio_space;
    IOExampleList io_example_list;
    DifferentProgramGenerator* g;
    int num_limit;
    FiniteInteractiveLearnSy(Specification* spec, GrammarEquivalenceChecker* _checker, Scorer* scorer,
                             DifferentProgramGenerator* g, int _num_limit=1000);
    virtual Example getNextExample(const PProgram& x, const PProgram& y);
    virtual void addExample(const IOExample& example);
    ~FiniteInteractiveLearnSy();
};

/**
 * @brief: LearnSy for interactive tasks where the input space is described by a Z3 formula.
 * The Z3 formula describing the input space is specified by program example_cons in the constructor.
 *
 * @field inp_var_list records the free variables in the Z3 formula.
 * @field param_list records the Z3 expressions corresponding to each parameter of the synthesis target.
 * @field cons_list caches the constraints of a valid input.
 *
 * @method getNextExample(x, y)
 * In each iteration, LearnSy finds up to num_limit examples where x and y output differnetly by Z3. Then, it selects
 * the one with the best approximated objective value among them.
 */
class Z3InteractiveLearnSy: public CompleteSelector, public InteractiveLearnSy {
public:
    Z3IOExampleSpace* zio_space;
    Z3Extension* ext;
    z3::expr_vector inp_var_list, param_list;
    z3::expr_vector cons_list;
    int num_limit;
    Z3InteractiveLearnSy(Specification* spec, GrammarEquivalenceChecker* _checker, Scorer* scorer, Program* example_cons, int _num_limit);
    virtual Example getNextExample(const PProgram& x, const PProgram& y);
    virtual void addExample(const IOExample& example);
    ~Z3InteractiveLearnSy() = default;
};

namespace selector::random {
    extern const std::string KExampleNumLimitName;
}

#endif //ISTOOL_INTERACTIVE_LEARNSY_H

//
// Created by pro on 2022/5/3.
//

#ifndef ISTOOL_NON_INTERACTIVE_LEARNSY_H
#define ISTOOL_NON_INTERACTIVE_LEARNSY_H

#include "interactive_learnsy.h"

/**
 * @breif: The base class of LearnSy for non-interactive tasks.
 * The main difference between interactive tasks and non-interactive tasks is that the program space in an interactive
 * task is guaranteed to be finite but that in a non-interactive task may be infinite.
 * LearnSy uses an iterative way to deal with infinite program spaces. It maintains a height limit, considers only the
 * finite truncation of the grammar within the height limit, and relax the height limit when the program synthesized by
 * the PBE solver exceeds the height limit.
 *
 * @field builder rebuilds the grammar and re-performs flattening when the height limit changes, and field scorer_build
 * rebuilds the scorer for calculating the approximated objective value when the height limit changes.
 *
 * This class is almost the same as InteractiveLearnSy in "interactive_learnsy.h", except method getBestExampleId will
 * relax the height limit and rebuild the grammar & the scorer when program exceeds the height limit.
 */
class NonInteractiveLearnSy {
    void initScorer();
public:
    Env* env;
    Grammar* g;
    int current_depth, KExampleNumLimit;
    FlattenGrammarBuilder* builder;
    Scorer* scorer;
    ExampleList example_list;
    LearnedScorerBuilder scorer_builder;
    NonInteractiveLearnSy(Env* env, FlattenGrammarBuilder* builder, const LearnedScorerBuilder& _scorer_builder);
    int getBestExampleId(const PProgram& program, const ExampleList& candidate_list, int num_limit = 5);
    void addHistoryExample(const Example& inp);
    ~NonInteractiveLearnSy();
};

/**
 * @brief: LearnSy for non-interactive tasks where the input space is described by a set of input-output examples.
 * This class is almost the same as FiniteInteractiveLearnSy in "interactive_learnsy.h"
 */

class FiniteNonInteractiveLearnSy: public Verifier, public NonInteractiveLearnSy {
public:
    FiniteIOExampleSpace* io_space;
    IOExampleList io_example_list;
    FiniteNonInteractiveLearnSy(Specification* spec, FlattenGrammarBuilder* builder, const LearnedScorerBuilder& _scorer_builder);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    ~FiniteNonInteractiveLearnSy() = default;
};

/**
 * @brief: LearnSy for non-interactive tasks where the input space is described by is described by a Z3 formula.
 * This class is almost the same as Z3InteractiveLearnSy in "interactive_learnsy.h"
 */

class Z3RandomSemanticsSelector: public Z3Verifier, public NonInteractiveLearnSy {
public:
    Z3IOExampleSpace* z3_io_space;
    int num_limit;
    Z3RandomSemanticsSelector(Specification* spec, FlattenGrammarBuilder* builder, const LearnedScorerBuilder& _scorer_builder, int _num_limit);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    ~Z3RandomSemanticsSelector() = default;
};

#endif //ISTOOL_NON_INTERACTIVE_LEARNSY_H

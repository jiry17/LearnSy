//
// Created by pro on 2022/1/12.
//

#ifndef ISTOOL_SELECTOR_H
#define ISTOOL_SELECTOR_H

#include "istool/basic/env.h"
#include "istool/basic/verifier.h"
#include "istool/solver/solver.h"

/**
 * @brief: Record the number of used examples.
 * @method addExampleCount(): Increase example_count by 1.
 */
class ExampleCounter {
public:
    int example_count = 0;
    void addExampleCount();
};

/**
 * @brief: Check whether all remaining programs in the grammar are semantically equivalent.
 * This class is used for implementing a selector for interactive program synthesis, where a program is synthesized
 * only after all ambiguity has been excluded.
 *
 * @method addExample(example):
 * Take a new input-output example into consideration.
 *
 * @method getTwoDifferentPrograms():
 * Return a remaining program when all remaining programs in the grammar are semantically equivalent. Otherwise, return
 * any two remaining programs that are semantically different.
 */
class GrammarEquivalenceChecker {
public:
    virtual void addExample(const IOExample& example) = 0;
    virtual ProgramList getTwoDifferentPrograms() = 0;
    virtual ~GrammarEquivalenceChecker() = default;
};


/**
 * @breif: The base class of selectors for interactive tasks.
 * This question selector repeatedly selects examples until all remaining programs are semantically equivalent.
 * After the selection is finished, the remaining program is the synthesis result. Therefore, CompleteSelector is a
 * subclass of Solver.
 * Besides, the number of used examples should be recorded, hence it is also a subclass of ExampleCounter.
 *
 * @method getNextExample(x, y):
 * Select the next example after two semantically different programs x and y are provided.
 *
 * @method addExample(example):
 * Take a new input-output example into consideration.
 *
 * @method synthesis(guard):
 * Solve the synthesis task under the time limit guarded by guard.
 * In general, there may be multiple target programs in a synthesis task, hence the return type of this method is
 * FunctionContext, an alis for std::unordered_map<std::string, Program>. In this project, we consider only tasks with
 * a single target. Therefore, FunctionContext can be simply regarded as a single program.
 */
class CompleteSelector: public ExampleCounter, public Solver {
public:
    IOExampleSpace* io_space;
    GrammarEquivalenceChecker* checker;
    CompleteSelector(Specification* spec, GrammarEquivalenceChecker* _checker);
    virtual Example getNextExample(const PProgram& x, const PProgram& y) = 0;
    virtual void addExample(const IOExample& example) = 0;
    virtual FunctionContext synthesis(TimeGuard* guard = nullptr);
    virtual ~CompleteSelector();
};

/**
 * @brief: The base class of selectors for non-interactive tasks.
 * A question selector verifies the correctness of the synthesis result. When the result is incorrect, the selector
 * needs to select a proper input. Therefore, it is a subclass of Verifier.
 * Meanwhile, the number of used examples should be recorded, hence it is also a subclass of ExampleCounter.
 */
class Selector: public ExampleCounter, public Verifier {
public:
    virtual ~Selector() = default;
};

/**
 * @brief: Wrap a verifier to a question selector.
 * This class wraps a CEGIS-styled verifier to a question selector for non-interactive tasks, where the counter-example
 * returned by the verifier is directly returned as the selected example.
 */
class DirectSelector: public Selector {
public:
    Verifier* v;
    DirectSelector(Verifier* _v);
    virtual bool verify(const FunctionContext& info, Example* counter_example);
    ~DirectSelector();
};

#endif //ISTOOL_SELECTOR_H

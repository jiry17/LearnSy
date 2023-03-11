//
// Created by pro on 2023/1/2.
//
#include <cassert>
#include <iostream>
#include "istool/basic/type.h"

/**
 * In this example, we show how to apply LearnSy to a new synthesis domain.
 * We consider a simple synthesis problem related to integer lists, where the task is to synthesize a program "target"
 * that satisfies input-output examples [1, 2, 1] -> 1, [2, 3, 3] -> 3, and [1, 2, 3] -> 3.
 * The program space we considered is specified by the grammar below, where operator head takes the first element from
 * a non-empty list and returns 0 for an empty list, operator tail removes the first element from a list.
 *                    S[int] -> head(S[list]), S[list] -> Param0 | tail(S[list])
 * Clearly, program head(tail(tail(Param0))) is a valid program for this task.
 */

/**
 * First, we need to declare those types, values, and operators in the synthesis domain. In this example, we use lists
 * and integers. Those classes related to integers have been declared in "istool/parser/theory/basic/clia/clia.h", and
 * thus we can focus on those classes for lists here.
 * In the following, we declare the type of lists as class TList, declare a value of type TList (i.e., an integer list)
 * as ListValue, and then declare the correspondence between TList and ListValue via ListValueTypeInfo.
 */

class TList: public SimpleType {
public:
    virtual std::string getName() {
        return "List";
    }

    /**
     * This method is introduced by the PISTool library and is never used in this project. It is used to construct a
     * new type that has the same form as the current one, where the parameters of type abstractions may be changed.
     */
    virtual PType clone(const TypeList& params) {
        return std::make_shared<TList>();
    }

    virtual ~TList() = default;
};

#include "istool/basic/value.h"

class ListValue: public Value {
public:
    std::vector<int> elements;

    ListValue(const std::vector<int>& _elements): elements(_elements) {
    }

    /**
     * Convert a list value to strings. It would be better to ensure that different lists correspond to different
     * strings, because some synthesizers/selectors will use the string representation as hash keys.
     */
    virtual std::string toString() const {
        std::string res = "[";
        for (int i = 0; i < elements.size(); ++i) {
            if (i) res += ",";
            res += std::to_string(elements[i]);
        }
        return res + "]";
    }

    virtual bool equal(Value* value) const {
        auto* lv = dynamic_cast<ListValue*>(value);
        if (!lv || lv->elements.size() != elements.size()) return false;
        for (int i = 0; i < elements.size(); ++i) {
            if (lv->elements[i] != elements[i]) return false;
        }
        return true;
    }

    virtual ~ListValue() = default;
};

#include "istool/basic/type_system.h"

/**
 * Each subclass of ValueTypeInfo assigns types to some values.
 */
class ListValueTypeInfo: public ValueTypeInfo {
public:
    /**
     * This method declares that ListValueTypeInfo can assign types to instances of ListValue.
     */
    virtual bool isMatch(Value* value) {
        return dynamic_cast<ListValue*>(value);
    }

    /**
     * The input of this method ensures isMatch(value). Here, it must be an instance of ListValue, and thus its type
     * should be TList.
     */
    virtual PType getType(Value* value) {
        return std::make_shared<TList>();
    }
};

#include "istool/basic/semantics.h"
#include "istool/parser/theory/basic/clia/clia.h"

class HeadSemantics: public NormalSemantics {
public:
    /**
     * The three parameters for constructing NormalSemantics declare the name, the output type, and the input types of
     * an operator, respectively.
     */
    HeadSemantics(): NormalSemantics("head", std::make_shared<TInt>(), {std::make_shared<TList>()}) {
    }

    /**
     * An instance of ExecuteInfo records the context information that may be used by some operators, such as the
     * values of the input variables. In this example, info is never used.
     */
    Data run(DataList&& inp, ExecuteInfo* info) {
        auto* lv = dynamic_cast<ListValue*>(inp[0].get());
        if (lv->elements.empty()) return BuildData(Int, 0);
        return BuildData(Int, lv->elements[0]);
    }
};

class TailSemantics: public NormalSemantics {
public:
    TailSemantics(): NormalSemantics("tail", std::make_shared<TList>(), {std::make_shared<TList>()}) {
    }

    Data run(DataList&& inp, ExecuteInfo* info) {
        auto* lv = dynamic_cast<ListValue*>(inp[0].get());
        std::vector<int> res;
        for (int i = 1; i < lv->elements.size(); ++i) res.push_back(lv->elements[i]);
        return BuildData(List, res);
    }
};

#include "istool/basic/specification.h"

/**
 * Second, we need to declare the synthesis task we considered as an instance of Specification.
 * A specification is formed by three parts: (1) the synthesis information (SynthInfo) for each target program,
 * including the name, the input/output types, and the grammar specifying the program space, (2) an environment (Env)
 * storing those types, operators, and, if necessary, other utilities (such as Z3 interface and witness functions), and
 * (3) an example space (ExampleSpace) declaring the semantical constraints.
 * In this task, there is only a single target program, and the example space is declared by a set of input-output
 * examples. The following function generates a specification corresponding to a given list of input-output examples.
 */
Specification* buildSynthesisTask(const IOExampleList& examples) {
    auto env = std::make_shared<Env>();
    // Load those pre-defined types, values, and operators related to integers into env.
    theory::loadCLIATheory(env.get());
    // Load the two list-related operators head and tail into env.
    env->setSemantics("head", std::make_shared<HeadSemantics>());
    env->setSemantics("tail", std::make_shared<TailSemantics>());
    // Load the correspondence between TList and ListValue.
    auto* type_ext = type::getTypeExtension(env.get());
    type_ext->registerTypeInfo(new ListValueTypeInfo());

    // Construct grammar S[int] -> head(S[list]), S[list] -> Param0 | tail(S[list])
    PType int_type = std::make_shared<TInt>(), list_type = std::make_shared<TList>();
    auto* int_symbol = new NonTerminal("S[int]", int_type);
    auto* list_symbol = new NonTerminal("S[list]", list_type);
    int_symbol->rule_list.push_back(new ConcreteRule(env->getSemantics("head"), {list_symbol}));
    list_symbol->rule_list.push_back(new ConcreteRule(env->getSemantics("tail"), {list_symbol}));
    list_symbol->rule_list.push_back(new ConcreteRule(semantics::buildParamSemantics(0, list_type), {}));
    auto* grammar = new Grammar(int_symbol, {int_symbol, list_symbol});

    // Construct the synthesis info for the target program "target".
    std::string target_name = "target";
    auto synth_info = std::make_shared<SynthInfo>(target_name, (TypeList){list_type}, int_type, grammar);

    // Construct an example space formed by a list of input-output examples.
    auto example_space = example::buildFiniteIOExampleSpace(examples, target_name, env.get());

    // Construct the specification from the synthesis information, the environment, and the example space.
    return new Specification({synth_info}, env, example_space);
}

#include "istool/solver/solver.h"

/**
 * Third, we implement a PBE solver for this synthesis task, which synthesizes a program from a set of input-output
 * examples (@method synthesis(example_list, guard)).
 * In the grammar we considered, all programs are in the form of head(tail(...tail(Param0))). When there are k "tail"s
 * used, the program outputs the (k+1)th element in the list. Therefore, we can synthesize a program by enumerating
 * the value of k and then checking whether the current k satisfies all examples.
 */
class ListSolver: public PBESolver {
public:
    std::string target_name;
    IOExampleSpace* io_example_space;
    ListSolver(Specification* _spec): PBESolver(_spec) {
        /**
         * Our solver consider only the tasks where (1) only a single synthesis target is involved, and (2) the
         * example space can generate input-output examples, i.e., an instance of IOExampleSpace.
         */
        io_example_space = dynamic_cast<IOExampleSpace*>(spec->example_space.get());
        assert(spec->info_list.size() == 1 && io_example_space);
        target_name = spec->info_list[0]->name;
    }

    Data getKthElement(ListValue* value, int k) {
        if (value->elements.size() <= k) return BuildData(Int, 0);
        return BuildData(Int, value->elements[k]);
    }

    virtual FunctionContext synthesis(const std::vector<Example>& example_list, TimeGuard* guard = nullptr) {
        IOExampleList io_example_list;
        for (auto& example: example_list) {
            io_example_list.push_back(io_example_space->getIOExample(example));
        }

        // Enumerate the value of k, and check whether it satisfies all examples.
        int k = 0;
        for (;; k++) {
            bool is_valid = true;
            for (auto& [inp, oup]: io_example_list) {
                if (!(getKthElement(dynamic_cast<ListValue*>(inp[0].get()), k) == oup)) {
                    is_valid = false; break;
                }
            }
            if (is_valid) break;
        }

        // Construct a program that uses operator tail exactly k times.
        PProgram res = program::buildParam(0);
        for (int i = 1; i <= k; ++i) {
            res = std::make_shared<Program>(spec->env->getSemantics("tail"), (ProgramList){res});
        }
        res = std::make_shared<Program>(spec->env->getSemantics("head"), (ProgramList){res});
        return semantics::buildSingleContext(target_name, res);
    }
};

#include "istool/selector/learnsy/non_interactive_learnsy.h"
/**
 * Fourth, we construct an instance of LearnSy to select examples for this task.
 * The synthesis task here is non-interactive, and its example space is specified by a finite list of input-output
 * examples. Therefore, we can use FiniteNonInteractiveLearnSy as the selector, which requires a FlattenGrammarBuilder
 * to flatten the original grammar, and a ScorerBuilder to construct the scorer each time when the flattened grammar
 * changes.
 * The following function constructs a default instance of FiniteNonInteractiveLearnSy for specification spec.
 */
Selector* buildLearnSy(Specification* spec) {
    // Declare a trivial PRTG where rules expanded from the same non-terminal have the same probability.
    auto *model = ext::vsa::getSizeModel();
    auto *grammar = spec->info_list[0]->grammar;
    int flatten_num = 10;
    // Construct a FlattenGrammarBuilder
    auto *fg_builder = new TrivialFlattenGrammarBuilder(grammar, model, spec->env.get(), flatten_num,
                                                        new AllValidProgramChecker());
    // Construct a ScorerBuilder
    LearnedScorerBuilder scorer_builder = [](Env *env, FlattenGrammar *fg) -> Scorer * {
        auto *holder = new BasicSampleStructureHolder(env, fg);
        auto *learner = new FixedSampleUnifiedEquivalenceModelLearner(holder);
        return new OptimizedScorer(env, fg, learner);
    };

    auto* learnsy = new FiniteNonInteractiveLearnSy(spec, fg_builder, scorer_builder);

    /**
     * The implementation of LearnSy will not record the number of used examples itself. Therefore, we need to wrap it
     * with DirectSelector to get the number of used examples.
     */
    return new DirectSelector(learnsy);
}

/**
 * Finally, we combine the PBE solver (ListSolver) and LearnSy into a CEGIS solver and then apply this solver to solve
 * the synthesis task considered in this example.
 * After building this project, the executable file of this example can be found as `builder/example/list_example`
 */
int main(int argc, char** argv) {
    // Declare the list of input-output examples.
    std::vector<int> inp1 = {1, 2, 1}, inp2 = {2, 3, 3}, inp3 = {1, 2, 3};
    int oup1 = 1, oup2 = 3, oup3 = 3;
    IOExample example1({BuildData(List, inp1)}, BuildData(Int, oup1));
    IOExample example2({BuildData(List, inp2)}, BuildData(Int, oup2));
    IOExample example3({BuildData(List, inp3)}, BuildData(Int, oup3));

    // Construct the specification, the PBE solver, and LearnSy.
    auto* spec = buildSynthesisTask({example1, example2, example3});
    auto* pbe_solver = new ListSolver(spec);
    auto* learnsy = buildLearnSy(spec);

    // Combine the PBE solver and LearnSy into a CEGIS solver.
    auto* solver = new CEGISSolver(pbe_solver, learnsy);

    // Solve the synthesis task.
    auto res = solver->synthesis();
    std::cout << "Synthesis result: " << res.toString() << std::endl;
    std::cout << "#Example: " << learnsy->example_count << std::endl;
}
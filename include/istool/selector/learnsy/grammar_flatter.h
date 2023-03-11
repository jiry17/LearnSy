//
// Created by pro on 2022/5/2.
//

#ifndef ISTOOL_GRAMMAR_FLATTER_H
#define ISTOOL_GRAMMAR_FLATTER_H

#include "istool/basic/grammar.h"
#include "equivalence_checker_tool.h"
#include "istool/solver/maxflash/topdown_context_graph.h"

/**
 * @brief: The result of performing flattening.
 * This class records the flattened grammar and maintains the correspondences between the flattened grammar and the
 * original grammar.
 *
 * @field graph records the flattened grammar, where the probabilities are assigned to each rule.
 * @field param_info_list records the original programs corresponding to each terminated rule introduced by flattening.
 * @class TopDownGraphMatchStructure records how a program is expanded from the flattened grammar.
 *
 * @method getMatchStructure(program):
 * Recover how program is expanded from the flattened grammar.
 *
 * @method getFlattenInput(input):
 * Given an input, calculate the value of each terminated rule in the flattened grammar.
 */
class FlattenGrammar {
protected:
    virtual TopDownGraphMatchStructure* getMatchStructure(int node_id, const PProgram& program) const = 0;
public:
    struct ParamInfo {
        PType type;
        PProgram program;
        ParamInfo(const PType& _type, const PProgram& _program);
        ParamInfo() = default;
    };
    TopDownContextGraph* graph;
    std::vector<ParamInfo> param_info_list;
    std::unordered_map<std::string, TopDownGraphMatchStructure*> match_cache;
    Env* env;
    void print() const;
    TopDownGraphMatchStructure* getMatchStructure(const PProgram& program);
    Example getFlattenInput(const Example& input) const;
    FlattenGrammar(TopDownContextGraph* _graph, Env* _env);
    ~FlattenGrammar();
};

/**
 * @breif: The base class for performing flattening.
 *
 * @method getFlattenGrammar(depth):
 * Flattening grammar (together with PRTG model) after truncating the grammar with limit depth.
 */
class FlattenGrammarBuilder {
public:
    Grammar* grammar;
    TopDownModel* model;
    FlattenGrammarBuilder(Grammar* _grammar, TopDownModel* _model);
    virtual FlattenGrammar* getFlattenGrammar(int depth) = 0;
    virtual ~FlattenGrammarBuilder() = default;
};

/**
 * @breif: A flattened grammar where each flattened program corresponds to a separate terminated rule.
 * In such a flattened grammar, to check whether a program is expanded directly from a flattened rule, we need only to
 * check whether its syntax is equal to the program corresponding to some flattened rule.
 *
 * @field param_map records the mapping from the syntax to the index of the flattened rule.
 */
class TrivialFlattenGrammar: public FlattenGrammar {
protected:
    virtual TopDownGraphMatchStructure* getMatchStructure(int node_id, const PProgram& program) const;
public:
    std::unordered_map<std::string, int> param_map;
    TrivialFlattenGrammar(TopDownContextGraph* _g, Env* env, int flatten_num, ProgramChecker* validator);
    ~TrivialFlattenGrammar() = default;
};

/**
 * @breif: The builder for TrivialFlattenGrammar.
 *
 * @field flatten_num: The number limit of the grammar rules while performing flattening.
 * @field validator:
 * Check whether a program needs to be ignored while performing flattening. Some PBE solvers may ignore some programs
 * in the program space for efficiency. For example, those VSA-based synthesizers (e.g., MaxFlash) ignored those
 * programs that cannot be expanded by the witness functions. Therefore, we exclude those programs while flattening to
 * avoid them from influencing the question selection.
 */

class TrivialFlattenGrammarBuilder: public FlattenGrammarBuilder {
public:
    Env* env;
    int flatten_num;
    ProgramChecker* validator;
    TrivialFlattenGrammarBuilder(Grammar* g, TopDownModel* model, Env* _env, int _flatten_num, ProgramChecker* _validator);
    virtual FlattenGrammar* getFlattenGrammar(int depth);
    ~TrivialFlattenGrammarBuilder();
};

/**
 * @breif: A flattened grammar where semantically equivalent terminated rules are merged.
 * A more precise flattened grammar where equivalent rules are merged. Compared to TrivialFlattenGrammar, this grammar
 * can flatten more programs within the same limit for flattening, but constructing this grammar also suffers from a
 * higher time cost.
 * In such a flattened grammar, to check whether a program is expanded directly from a flattened rule, we need to check
 * whether this program is semantically equivalent to the program corresponding to some flattened rule. Such a check is
 * performed by EquivalenceCheckTool.
 */
class MergedFlattenGrammar: public FlattenGrammar {
protected:
    virtual TopDownGraphMatchStructure* getMatchStructure(int node_id, const PProgram& program) const;
public:
    std::unordered_map<std::string, int> param_map;
    PEquivalenceCheckTool tool;
    MergedFlattenGrammar(TopDownContextGraph* _g, Env* env, int flatten_num, ProgramChecker* validator, const PEquivalenceCheckTool& tool);
    ~MergedFlattenGrammar() = default;
};

/**
 * @brief: The builder for MergedFlattenGrammar.
 * This class is almost the same as TrivalFlattenGrammarBuilder.
 */

class MergedFlattenGrammarBuilder: public FlattenGrammarBuilder {
public:
    Env* env;
    int flatten_num;
    ProgramChecker* validator;
    PEquivalenceCheckTool tool;
    MergedFlattenGrammarBuilder(Grammar* g, TopDownModel* model, Env* _env, int _flatten_num,
            ProgramChecker* _validator, const PEquivalenceCheckTool& tool);
    virtual FlattenGrammar* getFlattenGrammar(int depth);
    virtual ~MergedFlattenGrammarBuilder();
};

#endif //ISTOOL_GRAMMAR_FLATTER_H

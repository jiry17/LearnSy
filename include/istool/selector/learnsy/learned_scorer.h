//
// Created by pro on 2022/6/7.
//

#ifndef ISTOOL_LEARNED_SCORER_H
#define ISTOOL_LEARNED_SCORER_H

#include "unified_equivalence_model.h"

/**
 * @breif: The base class for calculating the approximated objective value.
 *
 * @method learnModel(inp):
 * Learn a model for inp. The results of this method are cached in model_cache.
 *
 * @method pushExample(inp): Take a new input into consideration.
 * @method popExample(): Remove the earliest input to reduce the time cost.
 *
 * @method getScore(p, inp):
 * Calculate the approximated objective value for input inp when the program synthesized by the PBE solver is p.
 */
class Scorer {
public:
    UnifiedEquivalenceModelLearner* learner;
    std::vector<UnifiedEquivalenceModel*> model_list;
    std::unordered_map<std::string, UnifiedEquivalenceModel*> model_cache;
    Env* env; FlattenGrammar* fg;
    Scorer(Env* env, FlattenGrammar* fg, UnifiedEquivalenceModelLearner* _learner);
    UnifiedEquivalenceModel* learnModel(const Example& inp);
    virtual void pushExample(const Example& inp);
    virtual void popExample();
    virtual RandomSemanticsScore getScore(const PProgram& p, const Example& inp) = 0;
    virtual ~Scorer();
};
typedef std::function<Scorer*(Env*, FlattenGrammar*)> LearnedScorerBuilder;

/**
 * @brief: A cache item for accelerating calculating the approximated objective value.
 * In a flattened grammar, most grammar rules are terminated. While calculating the approximated objective value, the
 * calculation related only to terminated rules is simple. These calculations can be pre-performed to speed up the
 * calculation of the approximated objective value.
 *
 * @field node: The non-terminal considered by this cache item.
 * @field terminate_list: The terminated rules expanded from the considered non-terminal.
 *
 * @field match_storage:
 * For each two terminated rules, record the set of inputs where the two rules output the same. The set is compressed
 * into an integer using bit-operations.
 *
 * @field res_list:
 * For a terminated rule with index i, and a set of inputs S (expressed as an integer), res_list[i][S] records the
 * probability for two random programs x and y to output the same on S, where x is expanded from rule i, and y is
 * expanded from some terminated rule.
 *
 * @field full_res:
 * For a set of inputs S (expressed as an integer), full_res[S] records the probability for two random programs to
 * output the same on S, where both programs are expanded from terminated rules.
 *
 * @method getEqClass(inp): Get the equivalence classes of terminated rules on input inp.
 * @method pushExample(inp): Take a new input into consideration.
 * @method popExample(): Remove the earliest input to reduce the time cost.
 * @method getTerminateResult(inp): Return res_list after considering a new input inp.
 * @method getFullResult(inp, res): Score full_res into res after consdering a new input inp.
 */
class ScorerCacheItem {
    std::unordered_map<std::string, std::vector<int>> getEqClass(const DataList& inp);
public:
    TopDownContextGraph::Node* node;
    std::vector<TopDownContextGraph::Edge*> terminate_list;
    std::vector<std::vector<int>> match_storage;
    std::vector<RandomSemanticsScore*> res_list;
    RandomSemanticsScore* full_res;
    int example_num;
    void initResList();
    void pushExample(const DataList& inp);
    void popExample();
    std::vector<RandomSemanticsScore*> getTerminateResult(const DataList& inp);
    void getFullResult(const DataList& inp, RandomSemanticsScore* res);
    ScorerCacheItem(TopDownContextGraph::Node* _node);
    ~ScorerCacheItem();
};

/**
 * @breif: Store the cache item for each non-terminal.
 */
class ScorerCache {
public:
    TopDownContextGraph* graph;
    std::vector<ScorerCacheItem*> item_list;
    void pushExample(const DataList& inp);
    void popExample();
    std::vector<RandomSemanticsScore*> getTerminateResult(int node_id, const DataList& inp);
    void getFullResult(int node_id, const DataList& inp, RandomSemanticsScore* res);
    ScorerCache(TopDownContextGraph* _graph);
    ~ScorerCache();
};

/**
 * @brief: An implementation of Scorer that uses ScorerCache to speed up the calculation.
 */
class OptimizedScorer: public Scorer {
public:
    ScorerCache* cache_pool;
    virtual void pushExample(const DataList& inp);
    virtual void popExample();
    virtual RandomSemanticsScore getScore(const PProgram& p, const Example& inp);
    OptimizedScorer(Env* env, FlattenGrammar* fg, UnifiedEquivalenceModelLearner* _learner);
    virtual ~OptimizedScorer();
};

namespace selector::random {
    /**
     * An auxiliary function for calculating the approximated objective value.
     * @param weight_list: A list of length n.
     * @param res: A list of length $2^n$. Each index here corresponds to a subset of {0, ..., n-1}.
     * Let pre be the initial value of res. For each set S, this function updates res[S] to
     * $\sum_{T \subseteq S} pre[T] \prod{i \in S - T} weight_list[i] \prod{i \in T} (1 - weight_list[i])$
     * The time complexity of this function is $O(n2^n)$
     */
    void updateViolateProb(RandomSemanticsScore* res, const std::vector<RandomSemanticsScore>& weight_list);
    /**
     * An auxiliary function for calculating the approximated objective value.
     * @param weight_list: A list of length n.
     * @param res: A list of length $2^n$. Each index here represents a subset of {0, ..., n-1}.
     * Fr each set S, this function sets res[S] to $\prod_{i \in S} weight_list[i]$.
     * The time complexity of this function is $O(2^n)$.
     */
    void buildSetProduct(RandomSemanticsScore* res, const std::vector<RandomSemanticsScore>& weight_list);
    /**
     * Check whether edge corresponds to a terminated rule.
     */
    bool isTerminate(const TopDownContextGraph::Edge& edge);
    /**
     * Given a terminated rule edge, calculate its output on input inp.
     */
    Data getTerminateOutput(const TopDownContextGraph::Edge& edge, const DataList& inp);
}

#endif //ISTOOL_LEARNED_SCORER_H

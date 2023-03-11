//
// Created by pro on 2022/12/26.
//

#ifndef ISTOOL_UNIFIED_EQUIVALENCE_MODEL_H
#define ISTOOL_UNIFIED_EQUIVALENCE_MODEL_H

#include "istool/selector/selector.h"
#include "istool/selector/learnsy/grammar_flatter.h"
#include "istool/solver/maxflash/topdown_context_graph.h"
#include "istool/ext/vsa/vsa_extension.h"
#include "istool/ext/vsa/top_down_model.h"

typedef long double RandomSemanticsScore;

namespace selector::random {
    typedef float WeightType;
    typedef std::vector<std::vector<WeightType>> EqualWeightMatrix;
    typedef std::pair<Data, DataList> FullOutput;
}

/**
 * @breif: The class of unified-equivalence models.
 * Stores a unified-equivalence model learned on PRTG graph.
 *
 * @field weight_list: stores both self parameters and cross paramters.
 * 1. For a non-terminal s with index i and a grammar rule r expanded from s with index j, weight_list[i][j][j]
 *    records the value of parameter self[s](r).
 * 2. For a non-terminal s with index i and two grammar rules r1, r2 expanded from s with index j, k,
 *    weight_list[i][j][k] records the value of parameter cross[s](r1, r2).
 */
class UnifiedEquivalenceModel {
public:
    std::vector<selector::random::EqualWeightMatrix> weight_list;
    TopDownContextGraph* graph;
    UnifiedEquivalenceModel(TopDownContextGraph* _graph, const std::vector<selector::random::EqualWeightMatrix>& _weight_matrix);
    void print(bool is_nt=true);
    ~UnifiedEquivalenceModel() = default;
};

/**
 * @brief: The base class for learning a unified-equivalence model.
 * @field KSampleNum represents the number of samples used for learning.
 * @method learn(inp):
 * Learn a unified-equivalence model for input inp and flattened grammar fg.
 */
class UnifiedEquivalenceModelLearner {
public:
    FlattenGrammar* fg;
    TopDownContextGraph* graph;
    Env* env;
    int KSampleNum;
    UnifiedEquivalenceModelLearner(Env* _env, FlattenGrammar* _fg);
    virtual UnifiedEquivalenceModel* learn(const DataList& inp) = 0;
    virtual ~UnifiedEquivalenceModelLearner() = default;
};

/**
 * @brief: The base class for drawing samples for learning unified-equivalence model.
 * A trivial way to generate sample programs is to (1) enumerate all grammar rules, and then (2) randomly draw programs
 * from all programs expanded from each rule. However, such a procedure can be time-consuming since the size of a
 * flattened grammar may be large.
 * Here we utilize the recursive structure of the flattened grammar to speed up this procedure. In brief, for each
 * grammar rule s -> f(s1, ..., sk), we use the samples for non-terminals s1, ..., sk to construct the samples for this
 * grammar rule, while ensuring the distribution of each sample and the independency among samples for the same rule.
 *
 * @field sample_storage records the structures of samples.
 * @field dist_list records the distributions for randomly selecting a grammar rule for each non-terminal.
 * @field node_order records a topological order of non-terminals in the grammar, which represents the order for
 * performing sampling.
 *
 * @method buildStructure(node_id, edge_id, sub_list):
 * Construct a new sample for the (edge_id)th rule expanded from the (node_id)th non-terminal, where the sub-programs
 * are specified by those samples in sub_list. In this method, duplicated samples will be ignored (via sample_cache).
 *
 * @method setOutput(inp) calculates the outputs of all samples on input inp.
 * @method initSample(sample_num) performs sampling.
 */
class SampleStructureHolder {
public:
    struct SampleStructure {
        TopDownContextGraph::Edge* edge;
        std::vector<SampleStructure*> sub_list;
        selector::random::FullOutput oup;
        int index;
        SampleStructure(TopDownContextGraph::Edge* _edge, const std::vector<SampleStructure*>& _sub_list, int _index);
        Data execute(const DataList& inp);
        PProgram getProgram();
    };
    std::vector<std::vector<std::vector<SampleStructure*>>> sample_storage;
    std::vector<std::discrete_distribution<int>> dist_list;
    FlattenGrammar* fg;
    TopDownContextGraph* graph;
    std::vector<int> node_order;
    Env* env;
    int sample_index = 0;
    std::unordered_map<std::string, SampleStructure*> sample_cache;
    virtual SampleStructure* buildStructure(int node_id, int edge_id, const std::vector<SampleStructure*>& sub_list);
    virtual void setOutput(const DataList& inp) = 0;
    virtual void initSample(int sample_num) = 0;
    SampleStructureHolder(Env* _env, FlattenGrammar* _fg);
    virtual ~SampleStructureHolder();
};

/**
 * @brief: A trivial implementation of SampleStructureHolder where all samples will be retained.
 */
class BasicSampleStructureHolder: public SampleStructureHolder {
    SampleStructure* sampleProgram(int node_id);
    SampleStructure* sampleProgram(int node_id, int edge_id);
public:
    BasicSampleStructureHolder(Env* _env, FlattenGrammar* _fg);
    virtual void setOutput(const DataList& inp);
    virtual void initSample(int sample_num);
};

/**
 * @brief: An implementation of SampleStructureHolder where samples outside the VSA will be ignored.
 * Those VSA-based synthesizers (e.g., MaxFlash) ignored those programs that cannot be expanded by the witness
 * functions. Therefore, this holder ignores those samples that cannot be expanded by the witness functions and thus
 * avoids these samples from influencing the question selection.
 */
class VSASampleStructureHolder: public SampleStructureHolder {
    SampleStructure* sampleProgram(int node_id);
    SampleStructure* sampleProgram(int node_id, int edge_id);
    bool isInsideVSA(Semantics* sem, int example_id, const Data& oup, const DataList& inp);
    Data run(Semantics* sem, const DataList& sub_res, int inp_id);
    virtual SampleStructure* buildStructure(int node_id, int edge_id, const std::vector<SampleStructure*>& sub_list);
public:
    IOExampleList example_list;
    ExampleList flatten_input_list;
    VSAEnvSetter setter;
    Grammar* init_grammar;
    std::unordered_map<std::string, int> example_index_map;
    VSAExtension* ext;
    DataStorage oup_storage;
    std::unordered_map<std::string, WitnessList> witness_cache;
    VSASampleStructureHolder(Specification* spec, FlattenGrammar* _fg, const VSAEnvSetter& _setter);
    virtual void setOutput(const DataList& inp);
    virtual void initSample(int sample_num);
};

/**
 * @breif: An implementation of UnifiedEquivalenceModelLearner that based on samples.
 * This class generates a set of samples and then always uses this set of samples to learn the unified-equivalence
 * model for each considered input.
 */
class FixedSampleUnifiedEquivalenceModelLearner: public UnifiedEquivalenceModelLearner {
public:
    SampleStructureHolder* holder;
    FixedSampleUnifiedEquivalenceModelLearner(SampleStructureHolder* holder);
    virtual UnifiedEquivalenceModel* learn(const DataList& inp);
    ~FixedSampleUnifiedEquivalenceModelLearner();
};

namespace selector::random {
    extern const std::string KModelSampleNumName;
}

#endif //ISTOOL_UNIFIED_EQUIVALENCE_MODEL_H

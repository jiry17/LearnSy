//
// Created by pro on 2022/6/5.
//

#include "istool/parser/theory/basic/clia/clia_value.h"
#include "istool/selector/learnsy/unified_equivalence_model.h"
#include "glog/logging.h"
#include "istool/basic/config.h"
#include <iostream>
#include <cassert>

using namespace selector::random;

UnifiedEquivalenceModel::UnifiedEquivalenceModel(TopDownContextGraph *_graph, const std::vector<selector::random::EqualWeightMatrix> &_weight_matrix):
    graph(_graph), weight_list(_weight_matrix) {
}
namespace {
    bool _isNonTerminate(Semantics* sem) {
        return !dynamic_cast<ParamSemantics*>(sem) && !dynamic_cast<ConstSemantics*>(sem);
    }
}
void UnifiedEquivalenceModel::print(bool is_nt) {
    for (int node_id = 0; node_id < graph->node_list.size(); ++node_id) {
        auto& node = graph->node_list[node_id];
        std::cout << "node " << node.toString() << ":" << std::endl;
        std::vector<int> nt_list;
        if (is_nt) {
            for (int i = 0; i < node.edge_list.size(); ++i) {
                if (_isNonTerminate(node.edge_list[i].semantics.get())) {
                    nt_list.push_back(i);
                }
            }
        } else for (int i = 0; i < node.edge_list.size(); ++i)
            nt_list.push_back(i);
        for (auto edge_id: nt_list) std::cout << node.edge_list[edge_id].semantics->getName() << " ";
        std::cout << std::endl;
        for (auto i: nt_list) {
            for (auto j: nt_list) printf("%.5lf ", (double)weight_list[node_id][i][j]);
            std::cout << std::endl;
        }
    }
}

namespace {
    const int KDefaultSampleNum = 10;
}

#include <queue>
UnifiedEquivalenceModelLearner::UnifiedEquivalenceModelLearner(Env *_env, FlattenGrammar *_fg): env(_env), fg(_fg), graph(_fg->graph) {
    KSampleNum = theory::clia::getIntValue(*(env->getConstRef(KModelSampleNumName, BuildData(Int, KDefaultSampleNum))));
}

namespace {
    struct _EdgeResInfo {
    public:
        std::vector<RandomSemanticsScore> weight_list;
        std::vector<int> used_pos;
        RandomSemanticsScore sub_diff_equal_prob;
        _EdgeResInfo(const std::vector<RandomSemanticsScore>& _weight_list, const std::vector<int>& _used_pos,
                RandomSemanticsScore _sub_diff_equal_prob):
                weight_list(_weight_list), used_pos(_used_pos), sub_diff_equal_prob(_sub_diff_equal_prob) {
        }
        ~_EdgeResInfo() = default;
    };

    const RandomSemanticsScore KDefaultWeight = 0.05;

    RandomSemanticsScore _getSelfDiffProb(int eq_num, int sub_eq_num, int sample_num) {
        int total = sample_num * (sample_num - 1) / 2;
        assert(total >= eq_num && eq_num >= sub_eq_num);
        if (sub_eq_num == total) return KDefaultWeight;
        return (eq_num - sub_eq_num) / RandomSemanticsScore(total - sub_eq_num);
    }

    RandomSemanticsScore _getCrossProb(const _EdgeResInfo& x_info, const _EdgeResInfo& y_info) {
        if (x_info.used_pos.size() > y_info.used_pos.size()) return _getCrossProb(y_info, x_info);
        if (x_info.used_pos.empty()) return KDefaultWeight;
        RandomSemanticsScore res = 0.0;
        for (auto pos: x_info.used_pos) res += x_info.weight_list[pos] * y_info.weight_list[pos];
        return res;
    }
}

SampleStructureHolder::SampleStructure::SampleStructure(TopDownContextGraph::Edge *_edge, const std::vector<SampleStructure *> &_sub_list, int _index):
    edge(_edge), sub_list(_sub_list), index(_index) {
}
PProgram SampleStructureHolder::SampleStructure::getProgram() {
    ProgramList sub(sub_list.size());
    for (int i = 0; i < sub_list.size(); ++i) {
        sub[i] = sub_list[i]->getProgram();
    }
    return std::make_shared<Program>(edge->semantics, sub);
}
Data SampleStructureHolder::SampleStructure::execute(const DataList &inp) {
    if (!oup.first.isNull()) return oup.first;
    auto* ps = dynamic_cast<ParamSemantics*>(edge->semantics.get());
    if (ps) return oup.first = inp[ps->id];
    auto* cs = dynamic_cast<ConstSemantics*>(edge->semantics.get());
    if (cs) return oup.first = cs->w;
    auto* fs = dynamic_cast<FullExecutedSemantics*>(edge->semantics.get());
    if (fs) {
        oup.second.resize(sub_list.size());
        for (int i = 0; i < sub_list.size(); ++i) oup.second[i] = sub_list[i]->execute(inp);
        auto sub_res = oup.second;
        return oup.first = fs->run(std::move(sub_res), nullptr);
    }
    LOG(FATAL) << "Unsupported semantics " << edge->semantics->getName();
}
SampleStructureHolder::SampleStructureHolder(Env* _env, FlattenGrammar *_fg): env(_env), fg(_fg), graph(_fg->graph) {
    for (const auto& node: graph->node_list) {
        std::vector<RandomSemanticsScore> weight_list;
        for (const auto& edge: node.edge_list) {
            weight_list.push_back(ext::vsa::getTrueProb(edge.weight, graph->prob_type));
        }
        dist_list.emplace_back(weight_list.begin(), weight_list.end());
    }

    std::vector<int> out_list(graph->node_list.size(), 0);
    std::vector<std::vector<int>> rev_edge_list(graph->node_list.size());
    for (int i = 0; i < graph->node_list.size(); ++i) {
        for (auto& edge: graph->node_list[i].edge_list) {
            for (auto v: edge.v_list) {
                out_list[i]++; rev_edge_list[v].push_back(i);
            }
        }
    }
    std::queue<int> Q;
    for (int i = 0; i < graph->node_list.size(); ++i) if (out_list[i] == 0) Q.push(i);
    for (int _ = 0; _ < graph->node_list.size(); ++_) {
        assert(!Q.empty());
        int k = Q.front(); node_order.push_back(k); Q.pop();
        for (auto v: rev_edge_list[k]) {
            out_list[v]--; if (out_list[v] == 0) Q.push(v);
        }
    }
}
SampleStructureHolder::SampleStructure * SampleStructureHolder::buildStructure(int node_id, int edge_id, const std::vector<SampleStructure *> &sub_list) {
    std::string feature = std::to_string(node_id) + "@" + std::to_string(edge_id);
    for (auto* sub: sub_list) feature += "@" + std::to_string(sub->index);
    if (sample_cache.count(feature)) return sample_cache[feature];
    return sample_cache[feature] = new SampleStructure(&(graph->node_list[node_id].edge_list[edge_id]), sub_list, sample_index++);
}
SampleStructureHolder::~SampleStructureHolder() noexcept {
    for (auto& info: sample_cache) delete info.second;
}
SampleStructureHolder::SampleStructure * BasicSampleStructureHolder::sampleProgram(int node_id) {
    int edge_id = dist_list[node_id](env->random_engine);
    return sampleProgram(node_id, edge_id);
}
SampleStructureHolder::SampleStructure * BasicSampleStructureHolder::sampleProgram(int node_id, int edge_id) {
    auto* edge = &(graph->node_list[node_id].edge_list[edge_id]);
    if (edge->v_list.empty() && !(sample_storage[node_id][edge_id].empty())) return sample_storage[node_id][edge_id][0];
    std::vector<SampleStructureHolder::SampleStructure*> sub_list(edge->v_list.size());
    for (int i = 0; i < edge->v_list.size(); ++i) {
        int v = edge->v_list[i]; sub_list[i] = sampleProgram(v);
    }
    auto* res = buildStructure(node_id, edge_id, sub_list);
    sample_storage[node_id][edge_id].push_back(res);
    return res;
}
void BasicSampleStructureHolder::initSample(int sample_num) {
    sample_storage.clear(); sample_storage.resize(graph->node_list.size());
    for (auto node_id: node_order) {
        auto& node = graph->node_list[node_id];
        auto& node_sample_list = sample_storage[node_id];
        node_sample_list.resize(node.edge_list.size());
        for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
            auto& edge = node.edge_list[edge_id];
            int limit = edge.v_list.empty() ? 1 : sample_num;
            while (node_sample_list[edge_id].size() < limit) sampleProgram(node_id, edge_id);
        }
    }
}
BasicSampleStructureHolder::BasicSampleStructureHolder(Env *_env, FlattenGrammar *_fg):
    SampleStructureHolder(_env, _fg) {
}
void BasicSampleStructureHolder::setOutput(const DataList &inp) {
    auto full_output = fg->getFlattenInput(inp);
    for (auto& info: sample_cache) info.second->oup = {{}, {}};
    for (auto& info: sample_cache) info.second->execute(full_output);
}

VSASampleStructureHolder::VSASampleStructureHolder(Specification *spec, FlattenGrammar *_fg, const VSAEnvSetter& _setter):
    SampleStructureHolder(spec->env.get(), _fg), init_grammar(spec->info_list[0]->grammar), setter(_setter) {
    auto* fio_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (!fio_space) LOG(FATAL) << "VSASampleStructureHolder require FiniteIOExampleSpace";
    for (auto& example: fio_space->example_space) {
        example_list.push_back(fio_space->getIOExample(example));
    }
    for (int i = 0; i < example_list.size(); ++i) {
        example_index_map[data::dataList2String(example_list[i].first)] = i;
        flatten_input_list.push_back(fg->getFlattenInput(example_list[i].first));
    }
    ext = ext::vsa::getExtension(env);
}
Data VSASampleStructureHolder::run(Semantics *sem, const DataList &sub_res, int inp_id) {
    auto* ps = dynamic_cast<ParamSemantics*>(sem);
    if (ps) return flatten_input_list[inp_id][ps->id];
    auto* cs = dynamic_cast<ConstSemantics*>(sem);
    if (cs) return cs->w;
    auto* fs = dynamic_cast<FullExecutedSemantics*>(sem);
    if (fs) {
        DataList sub = sub_res;
        return fs->run(std::move(sub), nullptr);
    }
    LOG(FATAL) << "Unknown semantics " << sem->getName();
}
bool VSASampleStructureHolder::isInsideVSA(Semantics* sem, int example_id, const Data &oup, const DataList &inp) {
    if (dynamic_cast<ParamSemantics*>(sem)) return true;
    auto feature = std::to_string(example_id) + "@" + oup.toString() + "@" + sem->getName();
    WitnessList wit_list;
    if (witness_cache.count(feature)) wit_list = witness_cache[feature];
    else {
        setter(init_grammar, env, example_list[example_id]);
        wit_list = (witness_cache[feature] = ext->getWitness(sem, std::make_shared<DirectWitnessValue>(oup), example_list[example_id].first));
    }
    for (auto& wit_term: wit_list) {
        bool is_inside = true;
        for (int i = 0; i < wit_term.size(); ++i) if (!wit_term[i]->isInclude(inp[i])) {
            is_inside = false; break;
        }
        if (is_inside) return true;
    }
    return false;
}
SampleStructureHolder::SampleStructure * VSASampleStructureHolder::buildStructure(int node_id, int edge_id, const std::vector<SampleStructure *> &sub_list) {
    std::string feature = std::to_string(node_id) + "@" + std::to_string(edge_id);
    for (auto* sub: sub_list) feature += "@" + std::to_string(sub->index);
    if (sample_cache.count(feature)) return sample_cache[feature];
    auto* edge = &(graph->node_list[node_id].edge_list[edge_id]);
    DataList oup_list;
    for (int example_id = 0; example_id < flatten_input_list.size(); ++example_id) {
        DataList sub_res;
        for (auto* sub: sub_list) {
            // std::cout << sub->index << " " << oup_storage.size() << std::endl;
            sub_res.push_back(oup_storage[sub->index][example_id]);
        }
        Data oup = run(edge->semantics.get(), sub_res, example_id);
        if (!isInsideVSA(edge->semantics.get(), example_id, oup, sub_res)) {
            return sample_cache[feature] = nullptr;
        }
        oup_list.push_back(oup);
    }
    oup_storage.push_back(oup_list);
    auto* res = new SampleStructure(edge, sub_list, sample_index++);
    // std::cout << "new " << res->index << std::endl;
    return sample_cache[feature] = res;
}
SampleStructureHolder::SampleStructure * VSASampleStructureHolder::sampleProgram(int node_id) {
    // std::cout << "sample " << node_id << std::endl;
    auto edge_id = dist_list[node_id](env->random_engine);
    return sampleProgram(node_id, edge_id);
}
SampleStructureHolder::SampleStructure * VSASampleStructureHolder::sampleProgram(int node_id, int edge_id) {
    auto* edge = &(graph->node_list[node_id].edge_list[edge_id]);
    // std::cout << "sample " << node_id << " " << edge_id << " " << edge->semantics->getName() << std::endl;
    if (edge->v_list.empty() && !(sample_storage[node_id][edge_id].empty())) return sample_storage[node_id][edge_id][0];
    std::vector<SampleStructureHolder::SampleStructure*> sub_list(edge->v_list.size());
    for (int i = 0; i < edge->v_list.size(); ++i) {
        int v = edge->v_list[i]; sub_list[i] = sampleProgram(v);
        if (!sub_list[i]) {
            sample_storage[node_id][edge_id].push_back(nullptr);
            return nullptr;
        }
    }
    auto* res = buildStructure(node_id, edge_id, sub_list);
    sample_storage[node_id][edge_id].push_back(res);
    return res;
}
void VSASampleStructureHolder::initSample(int sample_num) {
    sample_storage.clear(); sample_storage.resize(node_order.size());
    for (auto node_id: node_order) {
        auto& node_sample_list = sample_storage[node_id];
        auto& node = graph->node_list[node_id];
        node_sample_list.resize(node.edge_list.size());
        for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
            int limit = node.edge_list[edge_id].v_list.empty() ? 1 : sample_num;
            // std::cout << node_id << " " << edge_id << std::endl;
            while (node_sample_list[edge_id].size() < limit) {
                // std::cout << "start" << std::endl;
                sampleProgram(node_id, edge_id);
            }
        }
    }
    for (auto& node_list: sample_storage) {
        for (auto& edge_list: node_list) {
            if (edge_list.size() > sample_num) edge_list.resize(sample_num);
            int now = 0;
            for (auto* s: edge_list) {
                if (s) edge_list[now++] = s;
            }
            edge_list.resize(now);
        }
    }
}
void VSASampleStructureHolder::setOutput(const DataList &inp) {
    auto feature = data::dataList2String(inp);
    if (example_index_map.count(feature) == 0) LOG(FATAL) << "Unknown input " << feature;
    auto id = example_index_map[feature];
    for (auto& info: sample_cache) {
        auto* s = info.second; if (!s) continue;
        s->oup.first = oup_storage[s->index][id]; s->oup.second.clear();
        for (auto* sub: s->sub_list) s->oup.second.push_back(oup_storage[sub->index][id]);
    }
}

FixedSampleUnifiedEquivalenceModelLearner::FixedSampleUnifiedEquivalenceModelLearner(SampleStructureHolder *_holder):
        UnifiedEquivalenceModelLearner(_holder->env, _holder->fg), holder(_holder) {
    holder->initSample(KSampleNum);
    LOG(INFO) << "sample num " << holder->sample_cache.size();
}

UnifiedEquivalenceModel * FixedSampleUnifiedEquivalenceModelLearner::learn(const DataList &inp) {
    holder->setOutput(inp);
    std::vector<EqualWeightMatrix> weight_matrix_list(graph->node_list.size());

    for (int node_id = 0; node_id < graph->node_list.size(); ++node_id) {
        std::unordered_map<std::string, int> oup_index_map;
        int num = 0; auto& node = graph->node_list[node_id];
        auto& node_sample_list = holder->sample_storage[node_id];
        std::vector<std::vector<int>> oup_storage(node.edge_list.size());
        std::vector<int> sub_eq_list(node.edge_list.size());
        for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
            auto& sample_list = node_sample_list[edge_id];
            std::unordered_map<std::string, int> sub_map;
            for (int i = 0; i < sample_list.size(); ++i) {
                auto* sample = sample_list[i];
                auto sub_feature = data::dataList2String(sample->oup.second);
                sub_eq_list[edge_id] += (sub_map[sub_feature]++);
                auto feature = sample->oup.first.toString();
                if (oup_index_map.count(feature) == 0) oup_index_map[feature] = num++;
                oup_storage[edge_id].push_back(oup_index_map[feature]);
            }
        }
        std::vector<int> tag(num, 0); int sign = 0;
        std::vector<_EdgeResInfo> edge_info_list;
        for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
            int sub_eq_num = sub_eq_list[edge_id], eq_num = 0; ++sign;
            if (oup_storage[edge_id].empty()) {
                edge_info_list.emplace_back(std::vector<RandomSemanticsScore>(num, 0), std::vector<int>(), KDefaultWeight);
                continue;
            }
            std::vector<int> used_list, frequency_list(num, 0);
            for (auto oup: oup_storage[edge_id]) {
                eq_num += (frequency_list[oup]++);
                if (tag[oup] != sign) {
                    tag[oup] = sign; used_list.push_back(oup);
                }
            }
            assert(eq_num <= oup_storage[edge_id].size() * (oup_storage[edge_id].size() - 1) / 2);
            std::vector<RandomSemanticsScore> weight_list(num);
            for (int i = 0; i < num; ++i) weight_list[i] = frequency_list[i] / RandomSemanticsScore(oup_storage[edge_id].size());
            edge_info_list.emplace_back(weight_list, used_list, _getSelfDiffProb(eq_num, sub_eq_num, oup_storage[edge_id].size()));
        }

        auto& matrix = weight_matrix_list[node_id];

        matrix.resize(node.edge_list.size(), std::vector<WeightType>(node.edge_list.size(), 0.0));
        for (int x_id = 0; x_id < node.edge_list.size(); ++x_id) {
            for (int y_id = 0; y_id < node.edge_list.size(); ++y_id) {
                if (x_id > y_id) matrix[x_id][y_id] = matrix[y_id][x_id];
                else if (x_id == y_id) matrix[x_id][y_id] = edge_info_list[x_id].sub_diff_equal_prob;
                else matrix[x_id][y_id] = _getCrossProb(edge_info_list[x_id], edge_info_list[y_id]);
            }
        }
    }

    auto* res = new UnifiedEquivalenceModel(graph, weight_matrix_list);
    return res;
}

FixedSampleUnifiedEquivalenceModelLearner::~FixedSampleUnifiedEquivalenceModelLearner() noexcept {
}

const std::string selector::random::KModelSampleNumName = "RandomSelector@SampleNum";
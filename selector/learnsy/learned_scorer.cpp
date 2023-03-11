//
// Created by pro on 2022/6/7.
//

#include "istool/selector/learnsy/learned_scorer.h"
#include "glog/logging.h"

Scorer::Scorer(Env *_env, FlattenGrammar *_fg, UnifiedEquivalenceModelLearner *_learner):
        env(_env), fg(_fg), learner(_learner) {
}
Scorer::~Scorer() noexcept {
    delete learner;
    for (auto& info: model_cache) delete info.second;
}
void Scorer::pushExample(const Example &inp) {
    model_list.push_back(learner->learn(inp));
}
void Scorer::popExample() {
    for (int i = 1; i < model_list.size(); ++i) model_list[i - 1] = model_list[i];
    model_list.pop_back();
}
UnifiedEquivalenceModel * Scorer::learnModel(const Example &inp) {
    auto feature = data::dataList2String(inp);
    if (model_cache.count(feature)) return model_cache[feature];
    return model_cache[feature] = learner->learn(inp);
}

void selector::random::updateViolateProb(RandomSemanticsScore *res, const std::vector<RandomSemanticsScore> &weight_list) {
    int n = weight_list.size();
    for (int i = 0; i < n; ++i) {
        for (int S = (1 << n) - 1; S >= 0; --S) {
            if (S & (1 << i)) {
                res[S] = res[S] * (1 - weight_list[i]) + res[S - (1 << i)] * weight_list[i];
            }
        }
    }
}

void selector::random::buildSetProduct(RandomSemanticsScore *res, const std::vector<RandomSemanticsScore> &weight_list) {
    res[0] = 1.0; int n = weight_list.size();
    for (int i = 0; i < n; ++i) res[1 << i] = weight_list[i];
    for (int S = 1; S < (1 << n); ++S) {
        int T = S & (-S); res[S] = res[T] * res[S - T];
    }
}
bool selector::random::isTerminate(const TopDownContextGraph::Edge &edge) {
    auto* sem = edge.semantics.get();
    return dynamic_cast<ParamSemantics*>(sem) || dynamic_cast<ConstSemantics*>(sem);
}
Data selector::random::getTerminateOutput(const TopDownContextGraph::Edge &edge, const DataList &inp) {
    auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
    if (ps) return inp[ps->id];
    auto* cs = dynamic_cast<ConstSemantics*>(edge.semantics.get());
    return cs->w;
}
//
// Created by pro on 2022/5/19.
//

#include "istool/selector/learnsy/equivalence_checker_tool.h"
std::string FiniteEquivalenceCheckerTool::getFeature(Program* p) {
    DataList res;
    for (const auto& inp: inp_pool) res.push_back(env->run(p, inp));
    return data::dataList2String(res);
}
FiniteEquivalenceCheckerTool::FiniteEquivalenceCheckerTool(Env* _env, FiniteIOExampleSpace* fio): env(_env) {
        for (auto& example: fio->example_space) {
            auto io_example = fio->getIOExample(example);
            inp_pool.push_back(io_example.first);
        }
}
PProgram FiniteEquivalenceCheckerTool::insertProgram(const PProgram& p) {
    auto feature = getFeature(p.get());
    if (feature_map.count(feature) == 0) feature_map[feature] = p;
    return feature_map[feature];
}
PProgram FiniteEquivalenceCheckerTool::queryProgram(const PProgram& p) {
    auto feature = getFeature(p.get());
    if (feature_map.count(feature) == 0) return {};
    return feature_map[feature];
}
Data FiniteEquivalenceCheckerTool::getConst(Program* p) {
    auto res = env->run(p, inp_pool[0]);
    for (auto& inp: inp_pool) {
        auto now = env->run(p, inp);
        if (!(now == res)) return {};
    }
    return res;
}

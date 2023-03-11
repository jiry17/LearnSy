//
// Created by pro on 2022/7/1.
//

#include "istool/selector/baseline/biased_bitvector_selector.h"
#include "istool/parser/theory/basic/bv/bv.h"
#include "glog/logging.h"

namespace {
    std::string _getFeature(const Data& d, int K) {
        auto* bv = dynamic_cast<BitVectorValue*>(d.get());
        if (!bv) return "";
        std::string res;
        for (int i = 0; i < K; ++i) res += std::to_string(bv->w[i]);
        return res;
    }

    std::string _getFeature(const DataList& inp, int K) {
        std::string res;
        for (auto& d: inp) res += _getFeature(d, K);
        return res;
    }
}

BiasedBitVectorSelector::BiasedBitVectorSelector(FiniteIOExampleSpace *_example_space, int _K):
    example_space(_example_space), K(_K) {
    for (int i = 0; i < example_space->example_space.size(); ++i) {
        io_example_list.push_back(example_space->getIOExample(example_space->example_space[i]));
    }
    for (auto& example: io_example_list) {
        auto feature = _getFeature(example.first, K);
        feature_list.push_back(feature);
    }
}

bool BiasedBitVectorSelector::verify(const FunctionContext &info, Example *counter_example) {
    int best_pos = -1, best_time = ++stamp;
    for (int i = 0; i < io_example_list.size(); ++i) {
        if (!example_space->satisfyExample(info, example_space->example_space[i])) {
            int now = time_stamp[feature_list[i]];
            if (now < best_time) {
                best_pos = i; best_time = now;
            }
        }
    }
    if (best_pos == -1) return true;
    if (counter_example) *counter_example = example_space->example_space[best_pos];
    time_stamp[feature_list[best_pos]] = stamp;
    return false;
}

namespace {
    void _collectAll(int pos, int K, int num, int bit_size, Z3Extension* ext, DataList& tmp, std::vector<DataList>& res) {
        if (pos == num) {
            res.push_back(tmp); return;
        }
        for (int i = 0; i < (1 << K); ++i) {
            Bitset x(bit_size, 0);
            for (int j = 0; j < K; ++j) if (i & (1 << j)) x.set(j, 1);
            tmp[pos] = BuildData(BitVector, x);
            _collectAll(pos + 1, K, num, bit_size, ext, tmp, res);
        }
    }

    std::vector<DataList> _collectAll(int K, int num, int bit_size, Z3Extension* ext) {
        std::vector<DataList> res;
        DataList tmp(num);
        _collectAll(0, K, num, bit_size, ext, tmp, res);
        return res;
    }
}
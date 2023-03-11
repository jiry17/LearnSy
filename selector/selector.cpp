//
// Created by pro on 2022/1/12.
//

#include "istool/selector/selector.h"
#include "istool/parser/theory/basic/clia/clia.h"
#include "istool/basic/config.h"
#include "glog/logging.h"
#include <cassert>

void ExampleCounter::addExampleCount() {
    example_count += 1;
}

CompleteSelector::CompleteSelector(Specification *spec, GrammarEquivalenceChecker* _checker): Solver(spec), checker(_checker) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "CompleteSelector can synthesize only a single program";
    }
    io_space = dynamic_cast<IOExampleSpace*>(spec->example_space.get());
    if (!io_space) {
        LOG(FATAL) << "CompleteSelector requires IOExampleSpace";
    }
}
FunctionContext CompleteSelector::synthesis(TimeGuard *guard) {
    int example_num = 0;
    while (1) {
        auto programs = checker->getTwoDifferentPrograms();
        if (programs.size() == 1) {
            return semantics::buildSingleContext(io_space->func_name, programs[0]);
        }
        assert(programs.size() == 2);
        auto example = getNextExample(programs[0], programs[1]);
        addExampleCount();
        auto io_example = io_space->getIOExample(example);
        LOG(INFO) << "Add #" << ++example_num << " example " << example::ioExample2String(io_example);
        checker->addExample(io_example);
        addExample(io_example);
    }
}
CompleteSelector::~CompleteSelector() noexcept {
    delete checker;
}

bool DirectSelector::verify(const FunctionContext &info, Example *counter_example) {
    auto res = v->verify(info, counter_example);
    if (!res && counter_example) addExampleCount();
    return res;
}
DirectSelector::DirectSelector(Verifier *_v): v(_v) {}
// TODO: Decide when to delete v
DirectSelector::~DirectSelector() {}
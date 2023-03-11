//
// Created by pro on 2022/5/25.
//

#include "istool/invoker/invoker.h"
#include "istool/solver/external/external_solver_list.h"

Solver * invoker::single::buildExternalEuSolver(Specification *spec, Verifier *v, const InvokeConfig &config) {
    auto* eusolver = new ExternalEuSolver(spec->env.get());
    auto* solver = new ExternalSyGuSPBESolver(spec, eusolver, false);
    auto* cegis = new CEGISSolver(solver, v);
    return cegis;
}
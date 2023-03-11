//
// Created by pro on 2022/5/25.
//

#include "istool/solver/external/external_solver_list.h"
#include "istool/basic/config.h"
#include "istool/parser/parser/json_util.h"
#include "istool/parser/parser/parser.h"
#include <fstream>
#include "glog/logging.h"

ExternalEuSolver::ExternalEuSolver(Env *env, const std::string &path): ExternalSolver(env, "EuSolver", path.empty() ? config::KEuSolverPath: path) {
}

namespace {
    std::string _readAllLines(const std::string& file) {
        std::ifstream inf(file.c_str(), std::ios::in);
        char ch; std::string res;
        while (inf.get(ch)) res += ch;
        inf.close();
        return res;
    }
}

FunctionContext ExternalEuSolver::invoke(const std::string &benchmark_file, TimeGuard *guard) {
    auto oup_file = solver::external::createRandomFile(".out");
    std::string command = install_path + "/run_eusolver " + benchmark_file + " > " + oup_file;
    int time_limit = -1;
    if (guard) time_limit = int(guard->getRemainTime());
    int memory_limit = solver::external::getExternalMemoryLimit(env);
    solver::external::runCommand(command, "", time_limit, memory_limit);
    // read result
    auto res = _readAllLines(oup_file);
    int pos = 0;
    while (pos < res.length() && res[pos] != '\n') pos++;
    if (pos == res.length()) {
        LOG(FATAL) << "Unexpected result returned by EuSolver: " << res;
    }
    res = res.substr(pos);
    auto* ouf = fopen(oup_file.c_str(), "w");
    fprintf(ouf, "%s\n", res.c_str());
    fclose(ouf);
    auto value = parser::getJsonForSyGuSFile(oup_file)[0];
    auto name = value[1].asString();
    auto program = json::getProgramFromJson(value, env);
    std::system(("rm " + oup_file).c_str());
    return semantics::buildSingleContext(name, program);
}
cmake_minimum_required(VERSION 3.5)
project(ISToolSolver)

aux_source_directory(enum ENUM_SRC)
aux_source_directory(vsa VSA_SRC)
aux_source_directory(stun STUN_SRC)
aux_source_directory(polygen POLY_SRC)
aux_source_directory(maxflash MAX_SRC)
aux_source_directory(external EXT_SOLVER_SRC)
add_library(solver_lib STATIC solver.cpp iterative_solver.cpp  ${ENUM_SRC} ${VSA_SRC} ${STUN_SRC} ${POLY_SRC}
        ${MAX_SRC} ${EXT_SOLVER_SRC})
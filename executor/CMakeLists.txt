cmake_minimum_required(VERSION 3.13)
project(ISToolExecutor)
cmake_policy(SET CMP0079 NEW)
aux_source_directory(invoker INVOKER_SRC)
add_library(invoker_lib STATIC ${INVOKER_SRC})
set(ALL_LIBS ${SRC_LIBS} invoker_lib)

add_executable(run_non_interactive run_non_interactive.cpp)
target_link_libraries(run_non_interactive ${ALL_LIBS} ${ALL_LIBS} ${THIRDPARTY_LIBS})

add_executable(run_interactive run_interactive.cpp)
target_link_libraries(run_interactive ${ALL_LIBS} ${ALL_LIBS} ${THIRDPARTY_LIBS})
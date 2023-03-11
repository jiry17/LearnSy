cmake_minimum_required(VERSION 3.13)
project(ISToolExecutor)
cmake_policy(SET CMP0079 NEW)

add_executable(run_example list_example.cpp)
target_link_libraries(run_example ${SRC_LIBS} ${SRC_LIBS} ${THIRDPARTY_LIBS})
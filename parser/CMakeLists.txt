cmake_minimum_required(VERSION 3.5)
project(ISToolSyGuS)

add_subdirectory(theory)
aux_source_directory(parser sygus_parser_src)
aux_source_directory(samplesy samplesy_parser_src)
add_library(sygus_parser_lib ${sygus_parser_src} sygus.cpp)
add_library(samplesy_parser_lib ${samplesy_parser_src} samplesy_dsl.cpp)
set(Parser_LIBS sygus_parser_lib sygus_theory_lib samplesy_parser_lib  PARENT_SCOPE)
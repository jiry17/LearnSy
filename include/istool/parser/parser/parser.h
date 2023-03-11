//
// Created by pro on 2021/12/8.
//

#ifndef ISTOOL_PARSER_H
#define ISTOOL_PARSER_H

#include "istool/parser/theory/theory.h"
#include "json/json.h"

namespace parser {
    extern bool KIsRemoveDuplicated;
    Specification* getSyGuSSpecFromFile(const std::string& path);
    Specification* getSyGuSSpecFromJson(const Json::Value& value);
    Json::Value getJsonForSyGuSFile(const std::string& path);
}


#endif //ISTOOL_PARSER_H

//
// Created by pro on 2022/2/12.
//

#include "istool/parser/theory/z3/bv/bv_z3.h"
#include "istool/parser/theory/z3/bv/bv_z3_semantics.h"
#include "istool/parser/theory/z3/bv/bv_z3_type.h"
#include "glog/logging.h"

void theory::loadZ3BV(Env *env) {
    theory::bv::loadZ3Semantics(env);
    theory::bv::loadZ3Type(env);
}
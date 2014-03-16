/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <bliss/graph.hh>

#include "formula.h"
#include "common.h"

VarId Variable::id_counter_ = 1;

extern void parse_string(string s);

Formula* Formula::Parse(string str) {
  parse_string(str);
  assert(m.only_formula());
  return m.only_formula();
}

void Formula::dump(int indent) {
  for (int i = 0; i < indent; ++i) {
    printf("   ");
  }
  printf("%p: %s\n", (void*)this, name().c_str());
  for (uint i = 0; i < child_count(); ++i) {
    child(i)->dump(indent + 1);
  }
}

Formula* Formula::neg() {
  return m.get<NotOperator>(this);
}

string Mapping::pretty(bool, vec<CharId>* params) {
  if (params) {
    return m.game().variables()[getValue(*params) - 1]->pretty();
  } else {
    return ident_ + "$" + std::to_string(param_id_);
  }
}

// Tseitin transformation of arbitrary formula to CNF
CnfFormula* Formula::ToCnf() {
  CnfFormula* r = new CnfFormula();
  TseitinTransformation(r, true);
  return r;
}

CnfFormula* Formula::ToCnf(vec<CharId>& param) {
  CnfFormula* r = new CnfFormula();
  r->set_build_for_params(&param);
  TseitinTransformation(r, true);
  r->set_build_for_params(nullptr);
  return r;
}

// return the variable corresponding to the node during Tseitin transformation
VarId Formula::tseitin_var(vec<CharId>*) {
  if (tseitin_var_ == 0) {
    tseitin_var_ = Variable::NewVarId();
  }
  return tseitin_var_;
}

/******************************************************************************
 * Tseitin transformation.
 */

void TseitinAnd(VarId thisVar, CnfFormula* cnf,
                const vec<VarId>& list, uint offset,
                bool negate = false) {
  int neg = negate ? -1 : 1;
  // X <-> AND(A1, A2, ..)
  // 1. (X | !A1 | !A2 | ...)
  vec<VarId> first;
  for (auto v = list.begin() + offset; v < list.end(); ++v) {
    first.push_back(neg * -(*v));
  }
  first.push_back(thisVar);
  cnf->addClause(first);
  // 2. (A1 | !X) & (A2 | !X) & ...
  for (auto v = list.begin() + offset; v < list.end(); ++v) {
    cnf->addClause({ -thisVar, neg * (*v) });
  }
}

void AndOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  // if on top level, all childs must be true - just recurse down
  if (!top) {
    TseitinAnd(tseitin_var(cnf->build_for_params()), cnf,
               tseitin_children(cnf->build_for_params()), 0);
  }
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, top);
  }
}

void OrOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  vec<VarId> first;
  for (auto& f: children_) {
    first.push_back(f->tseitin_var(cnf->build_for_params()));
  }

  if (!top) {
    // X <-> OR(A1, A2, ..)
    // (!X | A1 | A2 | ...) & (!A1 | X) & (!A2 | X) & ...
    auto thisVar = tseitin_var(cnf->build_for_params());
    first.push_back(-thisVar);
    cnf->addClause(first);
    for (auto& f: children_) {
      cnf->addClause({ thisVar, -f->tseitin_var(cnf->build_for_params()) });
    }
  } else {
    cnf->addClause(first);
  }
  // recurse down
  for (auto&f : children_) {
    f->TseitinTransformation(cnf, false);
  }
}

void NotOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  // X <-> (!Y)
  // (!X | !Y) & (X | Y)
  auto thisVar = tseitin_var(cnf->build_for_params());
  auto childVar = child_->tseitin_var(cnf->build_for_params());
  if (top) cnf->addClause({ thisVar });
  if (!this->isLiteral()) {
    cnf->addClause({ -thisVar, -childVar });
    cnf->addClause({ thisVar, childVar });
    child_->TseitinTransformation(cnf, false);
  }
}

void ImpliesOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var(cnf->build_for_params());
  auto leftVar = left_->tseitin_var(cnf->build_for_params());
  auto rightVar = right_->tseitin_var(cnf->build_for_params());
  if (top) {
    cnf->addClause({ -leftVar, rightVar });
  } else {
    // X <-> (L -> R)
    // (!X | !L | R) & (L | X) & (!R | X)
    cnf->addClause({ -thisVar, -leftVar, rightVar });
    cnf->addClause({ leftVar, thisVar });
    cnf->addClause({ -rightVar, thisVar });
  }
  left_->TseitinTransformation(cnf, false);
  right_->TseitinTransformation(cnf, false);
}

void EquivalenceOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var(cnf->build_for_params());
  auto leftVar = left_->tseitin_var(cnf->build_for_params());
  auto rightVar = right_->tseitin_var(cnf->build_for_params());
  if (top) {
    cnf->addClause({ -leftVar, rightVar });
    cnf->addClause({ leftVar, -rightVar });
  } else {
    // X <-> (L <-> R)
    // (X | L | R) & (!X | !L | R) & (!X | L | !R) & (X | !L | !R)
    cnf->addClause({ thisVar, leftVar, rightVar });
    cnf->addClause({ -thisVar, -leftVar, rightVar });
    cnf->addClause({ -thisVar, leftVar, -rightVar });
    cnf->addClause({ thisVar, -leftVar, -rightVar });
  }
  left_->TseitinTransformation(cnf, false);
  right_->TseitinTransformation(cnf, false);
}

// 'to' exclusively
void TseitinNumerical(VarId thisVar, CnfFormula* cnf,
                      bool at_least, bool at_most, uint value,
                      const vec<VarId>& children, uint offset) {
  // X <-> ( A1 & T1 ) | ( !A1 & T2 )
  // (X | !T1 | !A1) & (T1 | !X | !A1) & (A1 | X | !T2) & (A1 | T2 | !X)
  if (value == children.size() - offset) {
    if (at_least) TseitinAnd(thisVar, cnf, children, offset);
  } else if (value == 0) {
    if (at_most) TseitinAnd(thisVar, cnf, children, offset, true);
  } else {
    auto t1 = Variable::NewVarId(); // OP-(value-1) in the rest
    auto t2 = Variable::NewVarId(); // OP-value in the rest
    cnf->addClause({ thisVar, -t1, -children[offset] });
    cnf->addClause({ -thisVar, t1, -children[offset] });
    cnf->addClause({ thisVar, -t2, children[offset] });
    cnf->addClause({ -thisVar, t2, children[offset] });
    TseitinNumerical(t1, cnf, at_least, at_most, value - 1, children, offset + 1);
    TseitinNumerical(t2, cnf, at_least, at_most, value, children, offset + 1);
  }
}

void ExactlyOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var(cnf->build_for_params());
  if (top) cnf->addClause({ thisVar });
  TseitinNumerical(thisVar, cnf,
                   true, true, value_,
                   tseitin_children(cnf->build_for_params()), 0);
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, false);
  }
}

void AtLeastOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var(cnf->build_for_params());
  if (top) cnf->addClause({ thisVar });
  TseitinNumerical(thisVar, cnf,
                   true, false, value_,
                   tseitin_children(cnf->build_for_params()), 0);
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, false);
  }
}

void AtMostOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var(cnf->build_for_params());
  if (top) cnf->addClause({ thisVar });
  TseitinNumerical(thisVar, cnf,
                   false, true, value_,
                   tseitin_children(cnf->build_for_params()), 0);
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, false);
  }
}

void Formula::AddToGraph(bliss::Digraph& g,
                         vec<CharId>* params,
                         int parent) {
  if (isLiteral()) {
    // just create an edge from parent to the variable
    assert(parent > 0);
    auto neg = false;
    auto c = this;
    // natahni hranu z parenta sem
    if (dynamic_cast<NotOperator*>(c)) {
      neg = true;
      c = c->neg();
    }
    auto map = dynamic_cast<Mapping*>(c);
    auto var = dynamic_cast<Variable*>(c);
    if (map) {
      assert(params);
      g.add_edge(parent, 2*map->getValue(*params) - 2 + neg);
    } else {
      assert(var);
      g.add_edge(parent, 2*var->id() - 2 + neg);
    }
  } else {
    // create a new node for the operator and call recursively for children
    auto id = g.get_nof_vertices();
    g.add_vertex(node_id());
    if (parent > 0)
      g.add_edge(parent, id);
    for (uint i = 0; i < child_count(); i++)
      child(i)->AddToGraph(g, params, id);
  }
}
/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <bliss/graph.hh>

#include "formula.h"
#include "common.h"

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

string Mapping::pretty(bool, const vec<CharId>* params) {
  if (params) {
    return m.game().vars()[getValue(*params)]->pretty();
  } else {
    return ident_ + "$" + std::to_string(param_id_);
  }
}

// return the variable corresponding to the node during Tseitin transformation
VarId Formula::tseitin_var(PicoSolver& cnf) {
  if (tseitin_var_ == 0) {
    tseitin_var_ = cnf.NewVarId();
  }
  return tseitin_var_;
}

/******************************************************************************
 * Tseitin transformation.
 */

void TseitinAnd(VarId thisVar, PicoSolver& cnf,
                const vec<VarId>& list, uint limit,
                bool negate = false) {
  int neg = negate ? -1 : 1;
  // X <-> AND(A1, A2, ..)
  // 1. (X | !A1 | !A2 | ...)
  vec<VarId> first;
  for (uint i = 0; i < limit; i++) {
    first.push_back(neg * -list[i]);
  }
  first.push_back(thisVar);
  cnf.AddClause(first);
  // 2. (A1 | !X) & (A2 | !X) & ...
  for (uint i = 0; i < limit; i++) {
    cnf.AddClause({ -thisVar, neg * list[i] });
  }
}

void AndOperator::TseitinTransformation(PicoSolver& cnf, bool top) {
  // if on top level, all childs must be true - just recurse down
  if (!top) {
    TseitinAnd(tseitin_var(cnf), cnf,
               tseitin_children(cnf), children_.size());
  }
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, top);
  }
}

void OrOperator::TseitinTransformation(PicoSolver& cnf, bool top) {
  vec<VarId> first;
  for (auto& f: children_) {
    first.push_back(f->tseitin_var(cnf));
  }

  if (!top) {
    // X <-> OR(A1, A2, ..)
    // (!X | A1 | A2 | ...) & (!A1 | X) & (!A2 | X) & ...
    auto thisVar = tseitin_var(cnf);
    first.push_back(-thisVar);
    cnf.AddClause(first);
    for (auto& f: children_) {
      cnf.AddClause({ thisVar, -f->tseitin_var(cnf) });
    }
  } else {
    cnf.AddClause(first);
  }
  // recurse down
  for (auto&f : children_) {
    f->TseitinTransformation(cnf, false);
  }
}

void NotOperator::TseitinTransformation(PicoSolver& cnf, bool top) {
  // X <-> (!Y)
  // (!X | !Y) & (X | Y)
  auto thisVar = tseitin_var(cnf);
  auto childVar = child_->tseitin_var(cnf);
  if (top) cnf.AddClause({ thisVar });
  if (!this->isLiteral()) {
    cnf.AddClause({ -thisVar, -childVar });
    cnf.AddClause({ thisVar, childVar });
    child_->TseitinTransformation(cnf, false);
  }
}

void ImpliesOperator::TseitinTransformation(PicoSolver& cnf, bool top) {
  auto thisVar = tseitin_var(cnf);
  auto leftVar = left_->tseitin_var(cnf);
  auto rightVar = right_->tseitin_var(cnf);
  if (top) {
    cnf.AddClause({ -leftVar, rightVar });
  } else {
    // X <-> (L -> R)
    // (!X | !L | R) & (L | X) & (!R | X)
    cnf.AddClause({ -thisVar, -leftVar, rightVar });
    cnf.AddClause({ leftVar, thisVar });
    cnf.AddClause({ -rightVar, thisVar });
  }
  left_->TseitinTransformation(cnf, false);
  right_->TseitinTransformation(cnf, false);
}

void EquivalenceOperator::TseitinTransformation(PicoSolver& cnf, bool top) {
  auto thisVar = tseitin_var(cnf);
  auto leftVar = left_->tseitin_var(cnf);
  auto rightVar = right_->tseitin_var(cnf);
  if (top) {
    cnf.AddClause({ -leftVar, rightVar });
    cnf.AddClause({ leftVar, -rightVar });
  } else {
    // X <-> (L <-> R)
    // (X | L | R) & (!X | !L | R) & (!X | L | !R) & (X | !L | !R)
    cnf.AddClause({ thisVar, leftVar, rightVar });
    cnf.AddClause({ -thisVar, -leftVar, rightVar });
    cnf.AddClause({ -thisVar, leftVar, -rightVar });
    cnf.AddClause({ thisVar, -leftVar, -rightVar });
  }
  left_->TseitinTransformation(cnf, false);
  right_->TseitinTransformation(cnf, false);
}

void TseitinNumerical(VarId thisVar, PicoSolver& cnf,
                      bool at_least, bool at_most, uint value,
                      const vec<VarId>& children) {
  auto n = children.size();
  vec<vec<VarId>> vars(n + 1, vec<VarId>(value + 1, 0));
  vars[n][value] = thisVar;
  for (uint m = n; m > 0; m--) {
    if (m <= value && vars[m][m] != 0 && at_least) {
      assert(vars[m][m] != 0);
      TseitinAnd(vars[m][m], cnf, children, m); // all others are true
    }
    for (uint l = 1; l <= value && l < m; l++) {
      if (vars[m][l] == 0) continue;
      if (vars[m-1][l] == 0) vars[m-1][l] = cnf.NewVarId();
      if (vars[m-1][l-1] == 0) vars[m-1][l-1] = cnf.NewVarId();
      // vars[m][l] <-> (children[m] & vars[m-1][l]) | (!children[m] & vars[m-1][l-1])
      cnf.AddClause({ vars[m][l], -vars[m-1][l], children[m-1] });
      cnf.AddClause({ -vars[m][l], vars[m-1][l], children[m-1] });
      cnf.AddClause({ vars[m][l], -vars[m-1][l-1], -children[m-1] });
      cnf.AddClause({ -vars[m][l], vars[m-1][l-1], -children[m-1] });
    }
    if (vars[m][0] != 0 && at_most) {
      TseitinAnd(vars[m][0], cnf, children, m, true); // all others are false
    }
  }
}

void ExactlyOperator::TseitinTransformation(PicoSolver& cnf, bool top) {
  auto thisVar = tseitin_var(cnf);
  if (top) cnf.AddClause({ thisVar });
  TseitinNumerical(thisVar, cnf,
                   true, true, value_,
                   tseitin_children(cnf));
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, false);
  }
}

void AtLeastOperator::TseitinTransformation(PicoSolver& cnf, bool top) {
  auto thisVar = tseitin_var(cnf);
  if (top) cnf.AddClause({ thisVar });
  TseitinNumerical(thisVar, cnf,
                   true, false, value_,
                   tseitin_children(cnf));
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, false);
  }
}

void AtMostOperator::TseitinTransformation(PicoSolver& cnf, bool top) {
  auto thisVar = tseitin_var(cnf);
  if (top) cnf.AddClause({ thisVar });
  TseitinNumerical(thisVar, cnf,
                   false, true, value_,
                   tseitin_children(cnf));
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, false);
  }
}

// For parent < -1, add a new root with type parent.
void Formula::AddToGraph(bliss::Graph& g,
                         const vec<CharId>* params,
                         int parent) {
  if (fixed_) return;
  if (parent < 0) {
    g.add_vertex(parent);
    parent = g.get_nof_vertices() - 1;
  }

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
    // TODO: rewrite as member functions, simplify And/Or of 1 etc.
    auto id = g.get_nof_vertices();
    g.add_vertex(type_id());
    if (parent > 0)
      g.add_edge(parent, id);
    for (uint i = 0; i < child_count(); i++)
      child(i)->AddToGraph(g, params, id);
  }
}
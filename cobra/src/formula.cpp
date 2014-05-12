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
  printf("%p: %s (%s %s)\n", (void*)this, name().c_str(), fixed_ ? "fixed" : "not fixed", fixed_value_ ? "1" : "0");
  for (auto c: children_)
    c->dump(indent + 1);
}

Formula* Formula::neg() {
  return m.get<NotOperator>(this);
}

uint Formula::Size() const {
  uint r = 1;
  for (auto c: children_)
    r += c->Size();
  return r;
}


VarId Formula::tseitin_var(CnfSolver& cnf) {
  if (tseitin_var_ == 0) {
    tseitin_var_ = cnf.NewVarId();
  }
  return tseitin_var_;
}

vec<VarId> Formula::tseitin_children(CnfSolver& cnf) {
  vec<VarId> vars(children_.size(), 0);
  std::transform(children_.begin(), children_.end(), vars.begin(),
    [&](Formula* f){
      return f->tseitin_var(cnf);
    });
  return vars;
}

void Formula::ResetTseitinIds() {
  tseitin_var_ = 0;
  for (auto c: children_)
    c->ResetTseitinIds();
}

string Formula::pretty_join(string sep, bool utf8,
                            const vec<CharId>* params) {
  if (children_.empty()) return "()";
  string s = "(" + children_.front()->pretty(utf8, params);
  for (auto it = std::next(children_.begin()); it != children_.end(); ++it) {
    s += sep;
    s += (*it)->pretty(utf8, params);
  }
  s += ")";
  return s;
}

string Mapping::pretty(bool, const vec<CharId>* params) {
  if (params) {
    return m.game().vars()[getValue(*params)]->pretty();
  } else {
    return ident_ + "$" + std::to_string(param_id_);
  }
}

/*  Satisfied in a given model
 *
 */

bool AndOperator::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  for (auto c: children_) {
    if (!c->Satisfied(model, params)) return false;
  }
  return true;
}

bool OrOperator::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  for (auto c: children_) {
    if (c->Satisfied(model, params)) return true;
  }
  return false;
}

bool AtLeastOperator::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  uint sat = 0;
  for (auto c: children_) {
    sat += c->Satisfied(model, params);
    if (sat >= value_) return true;
  }
  return false;
}

bool AtMostOperator::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  uint sat = 0;
  for (auto c: children_) {
    sat += c->Satisfied(model, params);
  }
  return sat <= value_;
}

bool ExactlyOperator::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  uint sat = 0;
  for (auto c: children_) {
    sat += c->Satisfied(model, params);
  }
  return sat == value_;
}

bool EquivalenceOperator::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  return children_[0]->Satisfied(model, params) ==
         children_[1]->Satisfied(model, params);
}

bool ImpliesOperator::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  return !children_[0]->Satisfied(model, params) ||
          children_[1]->Satisfied(model, params);
}

bool NotOperator::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  return !children_[0]->Satisfied(model, params);
}

bool Mapping::Satisfied(const vec<bool>& model, const vec<CharId> params) {
  return model[getValue(params)];
}

bool Variable::Satisfied(const vec<bool>& model, const vec<CharId>) {
  assert((unsigned)id_ < model.size());
  return model[id_];
}

/*  Fixed variable propagation
 *
 */

void AndOperator::PropagateFixed(const vec<VarId>& fixed,
                                 const vec<CharId>* params) {
  fixed_ = false;
  bool fixed_all = true;
  non_fixed_childs_ = children_.size();
  for (auto c: children_) {
    c->PropagateFixed(fixed, params);
    if (c->fixed() == true && c->fixed_value()== false) {
      fixed_ = true;
      fixed_value_ = false;
      return;
    }
    fixed_all = fixed_all && c->fixed();
    non_fixed_childs_ -= c->fixed();
  }
  if (fixed_all) {
    fixed_ = true;
    fixed_value_ = true;
  }
}

void OrOperator::PropagateFixed(const vec<VarId>& fixed,
                                const vec<CharId>* params) {
  fixed_ = false;
  bool fixed_all = true;
  non_fixed_childs_ = children_.size();
  for (auto c: children_) {
    c->PropagateFixed(fixed, params);
    if (c->fixed() == true && c->fixed_value() == true) {
      fixed_ = true;
      fixed_value_ = true;
      return;
    }
    fixed_all = fixed_all && c->fixed();
    non_fixed_childs_ -= c->fixed();
  }
  if (fixed_all) {
    fixed_ = true;
    fixed_value_ = false;
  }
}

void AtLeastOperator::PropagateFixed(const vec<VarId>& fixed,
                                     const vec<CharId>* params) {
  uint t = 0, f = 0;
  for (auto c: children_) {
    c->PropagateFixed(fixed, params);
    if (c->fixed() == true) {
      if (c->fixed_value()) t++;
      else f++;
    }
  }
  fixed_ = false;
  sat_childs_ = t;
  if (t >= value_) {
    fixed_ = true;
    fixed_value_ = true;
  } else if (f > children_.size() - value_) {
    fixed_ = true;
    fixed_value_ = false;
  }
}


void AtMostOperator::PropagateFixed(const vec<VarId>& fixed,
                                    const vec<CharId>* params) {
  uint t = 0, f = 0;
  for (auto c: children_) {
    c->PropagateFixed(fixed, params);
    if (c->fixed() == true) {
      if (c->fixed_value()) t++;
      else f++;
    }
  }
  fixed_ = false;
  sat_childs_ = t;
  if (f >= children_.size() - value_) {
    fixed_ = true;
    fixed_value_ = true;
  } else if (t > value_) {
    fixed_ = true;
    fixed_value_ = false;
  }
}

void ExactlyOperator::PropagateFixed(const vec<VarId>& fixed,
                                     const vec<CharId>* params) {
  uint t = 0, f = 0;
  for (auto c: children_) {
    c->PropagateFixed(fixed, params);
    if (c->fixed() == true) {
      if (c->fixed_value()) t++;
      else f++;
    }
  }
  fixed_ = false;
  sat_childs_ = t;
  if (t + f == children_.size() && t == value_) {
    fixed_ = true;
    fixed_value_ = true;
  } else if (f > children_.size() - value_ || t > value_) {
    fixed_ = true;
    fixed_value_ = false;
  }
}

void EquivalenceOperator::PropagateFixed(const vec<VarId>& fixed,
                                         const vec<CharId>* params) {
  fixed_ = false;
  children_[0]->PropagateFixed(fixed, params);
  children_[1]->PropagateFixed(fixed, params);
  if (children_[0]->fixed() && children_[1]->fixed()) {
    fixed_ = true;
    fixed_value_ = (children_[0]->fixed_value() ==
                    children_[1]->fixed_value());
  }
}

void ImpliesOperator::PropagateFixed(const vec<VarId>& fixed,
                                     const vec<CharId>* params) {
  fixed_ = false;
  children_[0]->PropagateFixed(fixed, params);
  children_[1]->PropagateFixed(fixed, params);
  if (children_[0]->fixed() && children_[0]->fixed_value()== false) { // false -> ?
    fixed_ = true;
    fixed_value_ = true;
  } else if (children_[1]->fixed() && children_[1]->fixed_value()== true) { // ? -> true
    fixed_ = true;
    fixed_value_ = true;
  } else if (children_[0]->fixed() && children_[1]->fixed() &&  // false -> true
             children_[0]->fixed_value()== true && children_[1]->fixed_value()== false) {
    fixed_ = true;
    fixed_value_ = false;
  }
}

void NotOperator::PropagateFixed(const vec<VarId>& fixed,
                                 const vec<CharId>* params) {
  fixed_ = false;
  children_[0]->PropagateFixed(fixed, params);
  fixed_ = children_[0]->fixed();
  fixed_value_ = !children_[0]->fixed_value();
}

void Mapping::PropagateFixed(const vec<VarId>& fixed,
                             const vec<CharId>* params) {
  fixed_ = false;
  assert(params);
  if (std::count(fixed.begin(), fixed.end(), getValue(*params))) {
    fixed_ = true;
    fixed_value_ = true;
  } else if (std::count(fixed.begin(), fixed.end(), -getValue(*params))) {
    fixed_ = true;
    fixed_value_ = false;
  }
}

void Variable::PropagateFixed(const vec<VarId>& fixed,
                              const vec<CharId>*) {
  fixed_ = false;
  if (std::count(fixed.begin(), fixed.end(), id_)) {
    fixed_ = true;
    fixed_value_ = true;
  } else if (std::count(fixed.begin(), fixed.end(), -id_)) {
    fixed_ = true;
    fixed_value_ = false;
  }
}

/******************************************************************************
 * Tseitin transformation.
 */

void TseitinAnd(VarId thisVar, CnfSolver& cnf,
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

void AndOperator::TseitinTransformation(CnfSolver& cnf, bool top) {
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

void OrOperator::TseitinTransformation(CnfSolver& cnf, bool top) {
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

void NotOperator::TseitinTransformation(CnfSolver& cnf, bool top) {
  // X <-> (!Y)
  // (!X | !Y) & (X | Y)
  auto thisVar = tseitin_var(cnf);
  auto childVar = children_[0]->tseitin_var(cnf);
  if (top) cnf.AddClause({ thisVar });
  if (!this->isLiteral()) {
    cnf.AddClause({ -thisVar, -childVar });
    cnf.AddClause({ thisVar, childVar });
    children_[0]->TseitinTransformation(cnf, false);
  }
}

void ImpliesOperator::TseitinTransformation(CnfSolver& cnf, bool top) {
  auto thisVar = tseitin_var(cnf);
  auto leftVar = children_[0]->tseitin_var(cnf);
  auto rightVar = children_[1]->tseitin_var(cnf);
  if (top) {
    cnf.AddClause({ -leftVar, rightVar });
  } else {
    // X <-> (L -> R)
    // (!X | !L | R) & (L | X) & (!R | X)
    cnf.AddClause({ -thisVar, -leftVar, rightVar });
    cnf.AddClause({ leftVar, thisVar });
    cnf.AddClause({ -rightVar, thisVar });
  }
  children_[0]->TseitinTransformation(cnf, false);
  children_[1]->TseitinTransformation(cnf, false);
}

void EquivalenceOperator::TseitinTransformation(CnfSolver& cnf, bool top) {
  auto thisVar = tseitin_var(cnf);
  auto leftVar = children_[0]->tseitin_var(cnf);
  auto rightVar = children_[1]->tseitin_var(cnf);
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
  children_[0]->TseitinTransformation(cnf, false);
  children_[1]->TseitinTransformation(cnf, false);
}

void TseitinNumerical(VarId thisVar, CnfSolver& cnf,
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

void ExactlyOperator::TseitinTransformation(CnfSolver& cnf, bool top) {
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

void AtLeastOperator::TseitinTransformation(CnfSolver& cnf, bool top) {
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

void AtMostOperator::TseitinTransformation(CnfSolver& cnf, bool top) {
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

void Mapping::TseitinTransformation(CnfSolver& cnf, bool top) {
  if (top) {
    assert(cnf.build_for_params());
    cnf.AddClause({ getValue(*cnf.build_for_params()) });
  }
}

void Variable::TseitinTransformation(CnfSolver& cnf, bool top) {
  if (top) {
    cnf.AddClause({ id_ });
  }
}

/*
 * Add a formula tree to the symmetry graph.
 */

void Formula::AddToGraphRooted(bliss::Graph& g,
                               const vec<CharId>* params,
                               int root) {
  if (fixed_) return;
  g.add_vertex(root);
  AddToGraph(g, params, g.get_nof_vertices() - 1);
}

void Formula::AddToGraph(bliss::Graph& g,
                         const vec<CharId>* params,
                         int parent) {
  if (fixed_) return;
  auto id = g.get_nof_vertices();
  g.add_vertex(type_id());
  if (parent > 0)
    g.add_edge(parent, id);
  for (auto c: children_)
    c->AddToGraph(g, params, id);
}

void AndOperator::AddToGraph(bliss::Graph& g,
                         const vec<CharId>* params,
                         int parent) {
  if (fixed_) return;
  uint id;
  if (non_fixed_childs_ > 1) {
    id = g.get_nof_vertices();
    g.add_vertex(type_id());
    if (parent > 0) g.add_edge(parent, id);
  } else {
    id = parent;
  }
  for (auto c: children_)
    c->AddToGraph(g, params, id);
}

void OrOperator::AddToGraph(bliss::Graph& g,
                         const vec<CharId>* params,
                         int parent) {
  if (fixed_) return;
  uint id;
  if (non_fixed_childs_ > 1) {
    id = g.get_nof_vertices();
    g.add_vertex(type_id());
    if (parent > 0) g.add_edge(parent, id);
  } else {
    id = parent;
  }
  for (auto c: children_)
    c->AddToGraph(g, params, id);
}

void Mapping::AddToGraph(bliss::Graph& g,
                         const vec<CharId>* params,
                         int parent) {
  if (fixed_) return;
  assert(parent > 0);
  assert(params);
  g.add_edge(parent, getValue(*params) - 1);
}

void Variable::AddToGraph(bliss::Graph& g,
                          const vec<CharId>* params,
                          int parent) {
  if (fixed_) return;
  assert(parent > 0);
  g.add_edge(parent, id_ - 1);
}

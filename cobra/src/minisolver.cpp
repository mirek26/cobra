/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "formula.h"
#include "common.h"

#include <minisat/core/Solver.h>
#include "minisolver.h"

SolverStats MiniSolver::stats_ = SolverStats();

MiniSolver::MiniSolver(const vec<Variable*>& vars, Formula* restriction):
    vars_(vars) {
  // Reserve id's for original variables.
  for (uint i = 1; i < vars_.size(); i++) {
    auto x = minisat_.newVar(true, true);
    assert(static_cast<uint>(x) + 1 == i);
  }
  if (restriction) AddConstraint(restriction);
}

//------------------------------------------------------------------------------
// Adding constraints

void MiniSolver::AddClause(vec<VarId>& list) {
  Minisat::vec<Minisat::Lit> v;
  for (int i = 0; i < contexts_.size(); i++)
    v.push(~contexts_[i]);
  for (auto var: list)
    v.push(Minisat::mkLit(abs(var) - 1, var > 0));
  minisat_.addClause(v);
}

void MiniSolver::AddClause(std::initializer_list<VarId> list) {
  Minisat::vec<Minisat::Lit> v;
  for (int i = 0; i < contexts_.size(); i++)
    v.push(~contexts_[i]);
  for (auto var: list)
    v.push(Minisat::mkLit(abs(var) - 1, var > 0));
  minisat_.addClause(v);
}

//------------------------------------------------------------------------------

void MiniSolver::OpenContext() {
  contexts_.push(Minisat::mkLit(minisat_.newVar(), true));
}

void MiniSolver::CloseContext() {
  assert(contexts_.size() > 0);
  auto k = contexts_.last();
  contexts_.pop();
  minisat_.addClause(~k);
}

//------------------------------------------------------------------------------
// SAT solver stuff

bool MiniSolver::_MustBeTrue(VarId id) {
  assert(id > 0);
  contexts_.push(Minisat::mkLit(abs(id) - 1, false));
  auto r = !minisat_.solve(contexts_);
  contexts_.pop();
  return r;
}

bool MiniSolver::_MustBeFalse(VarId id) {
  assert(id > 0);
  contexts_.push(Minisat::mkLit(abs(id) - 1, true));
  auto r = !minisat_.solve(contexts_);
  contexts_.pop();
  return r;
}

vec<VarId> MiniSolver::_GetFixedVars() {
  vec<VarId> result;
  for (uint id = 1; id < vars_.size(); id++) {
    if (_MustBeTrue(id)) result.push_back(id);
    if (_MustBeFalse(id)) result.push_back(-id);
  }
  return result;
}

uint MiniSolver::_GetNumOfFixedVars() {
  uint r = 0;
  for (uint id = 1; id < vars_.size(); id++) {
    r += _MustBeTrue(id);
    r += _MustBeFalse(id);
  }
  return r;
}

bool MiniSolver::_Satisfiable() {
  auto r = minisat_.solve(contexts_);
  return r;
}

bool MiniSolver::_OnlyOneModel() {
  _Satisfiable();
  vec<bool> ass = GetAssignment();
  OpenContext();

  vec<VarId> clause;
  for (uint id = 1; id < vars_.size(); id++) {
    clause.push_back(id * (ass[id] ? -1 : 1));
  }
  AddClause(clause);

  auto s = _Satisfiable();
  CloseContext();
  return !s;
}

vec<bool> MiniSolver::GetAssignment() {
  vec<bool> result(vars_.size(), false);
  for (uint id = 1; id < vars_.size(); id++) {
    if (Minisat::toInt(minisat_.modelValue(id - 1)) == 1) result[id] = true;
  }
  return result;
}

void MiniSolver::PrintAssignment() {
  vec<int> trueVar;
  vec<int> falseVar;
  printf("ASS :");
  for (uint id = 0; id < vars_.size(); id++) {
    printf("%i ", Minisat::toInt(minisat_.modelValue(id)));
  }
  printf("\n");
}

void MiniSolver::ForAllModels(VarId var, std::function<void()> callback){
  assert(var > 0 && (unsigned)var < vars_.size());
  for (auto v: std::initializer_list<Minisat::Lit>(
        {Minisat::mkLit(var - 1, true), Minisat::mkLit(var - 1, false)})) {
    contexts_.push(v);
    if (_Satisfiable()) {
      if ((unsigned)var == vars_.size() - 1) {
        callback();
      } else {
        ForAllModels(var + 1, callback);
      }
    }
    contexts_.pop();
  }
}

uint MiniSolver::_NumOfModels() {
  uint k = 0;
  ForAllModels(1, [&](){ k++; });
  return k;
}

vec<vec<bool>> MiniSolver::_GenerateModels() {
  vec<vec<bool>> models;
  ForAllModels(1, [&](){
    models.push_back(GetAssignment());
  });
  return models;
}

/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string>
#include "./formula.h"
#include "./simple-solver.h"
#include "./picosolver.h"

SolverStats SimpleSolver::stats_ = SolverStats();

SimpleSolver::SimpleSolver(uint var_count,
                           Formula* restriction) :
    restriction_(restriction) {
  var_count_ = var_count;
  PicoSolver sat(var_count, restriction);
  codes_ = sat.GenerateModels();
  for (uint i = 0; i < codes_.size(); i++) {
    sat_.push_back(i);
  }
  ready_ = true;
}

void SimpleSolver::AddConstraint(Formula* formula) {
  ready_ = false;
  constraints_.push_back(make_pair(formula, vec<CharId>()));
}

void SimpleSolver::AddConstraint(Formula* formula, const vec<CharId>& params) {
  ready_ = false;
  constraints_.push_back(make_pair(formula, params));
}

void SimpleSolver::OpenContext() {
  contexts_.push_back(constraints_.size());
  context_unsat_.push_back(vec<uint>());
}

void SimpleSolver::CloseContext() {
  assert(contexts_.size() > 0);
  // remove constraints added in this context
  auto k = contexts_.back();
  constraints_.erase(constraints_.begin() + k, constraints_.end());

  // add codes removed from sat_ in this context
  auto& removed = context_unsat_.back();
  sat_.insert(sat_.begin(), removed.begin(), removed.end());

  contexts_.pop_back();
  context_unsat_.pop_back();
  ready_ = false;
}

bool SimpleSolver::_MustBeTrue(VarId id) {
  if (!ready_) Update();
  for (auto& x : sat_)
    if (!codes_[x][id]) return false;
  return true;
}

bool SimpleSolver::_MustBeFalse(VarId id) {
  if (!ready_) Update();
  for (auto& x : sat_)
    if (codes_[x][id]) return false;
  return true;
}

uint SimpleSolver::_GetNumOfFixedVars() {
  return _GetFixedVars().size();
}

vec<VarId> SimpleSolver::_GetFixedVars() {
  if (!ready_) Update();
  vec<bool> canbe[2];
  canbe[0].resize(var_count_);
  canbe[1].resize(var_count_);
  for (auto x : sat_)
    for (uint i = 1; i < var_count_; i++)
      canbe[codes_[x][i]][i] = true;
  vec<VarId> result;
  for (uint i = 1; i < var_count_; i++) {
    if (!canbe[0][i]) result.push_back(i);
    if (!canbe[1][i]) result.push_back(-i);
  }
  return result;
}

bool SimpleSolver::TestSat(uint i) {
  assert(i < sat_.size());
  bool ok = true;
  for (auto& constr : constraints_) {
    if (!constr.first->Satisfied(codes_[sat_[i]], constr.second)) {
      ok = false;
      break;
    }
  }
  if (ok) return true;
  if (!context_unsat_.empty())
    context_unsat_.back().push_back(sat_[i]);
  sat_[i] = sat_.back();
  sat_.pop_back();
  return false;
}

void SimpleSolver::RemoveUntilSat(uint start) {
  while (sat_.size() > start && !TestSat(start))
    // TestSat removes the unsat code from sat
    {}
}

bool SimpleSolver::_Satisfiable() {
  RemoveUntilSat(0);
  return !sat_.empty();
}

bool SimpleSolver::_OnlyOneModel() {
  assert(sat_.size() > 0);
  RemoveUntilSat(1);
  return sat_.size() == 1;
}

vec<bool> SimpleSolver::GetModel() {
  assert(!sat_.empty());
  return codes_[sat_[0]];
}

uint SimpleSolver::_NumOfModels() {
  if (!ready_) Update();
  return sat_.size();
}

vec<vec<bool>> SimpleSolver::_GenerateModels() {
  if (!ready_) Update();
  vec<vec<bool>> result;
  for (auto x : sat_)
    result.push_back(codes_[x]);
  return result;
}

void SimpleSolver::Update() {
  for (int i = sat_.size() - 1; i >= 0; i--)
    TestSat(i);
  ready_ = true;
}

string SimpleSolver::pretty() {
  string s = restriction_->pretty(false) + " & ";
  for (auto& c : constraints_) {
    s += c.first->pretty(false, &c.second) + " & ";
  }
  s.erase(s.length()-3, 3);
  return s;
}

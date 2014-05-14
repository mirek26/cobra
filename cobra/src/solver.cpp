/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "./solver.h"
#include "./formula.h"

// Time-measuring wrappers

bool Solver::MustBeTrue(VarId id) {
  auto t1 = clock();
  auto result = _MustBeTrue(id);
  stats().fixed_calls++;
  stats().fixed_time += clock() - t1;
  return result;
}

bool Solver::MustBeFalse(VarId id) {
  auto t1 = clock();
  auto result = _MustBeFalse(id);
  stats().fixed_calls++;
  stats().fixed_time += clock() - t1;
  return result;
}

vec<VarId> Solver::GetFixedVars() {
  auto t1 = clock();
  auto result = _GetFixedVars();
  stats().fixed_calls++;
  stats().fixed_time += clock() - t1;
  return result;
}

uint Solver::GetNumOfFixedVars() {
  auto t1 = clock();
  auto result = _GetNumOfFixedVars();
  stats().fixed_calls++;
  stats().fixed_time += clock() - t1;
  return result;
}

bool Solver::Satisfiable() {
  auto t1 = clock();
  auto result = _Satisfiable();
  stats().sat_calls++;
  stats().sat_time += clock() - t1;
  return result;
}

bool Solver::OnlyOneModel() {
  auto t1 = clock();
  auto result = _OnlyOneModel();
  stats().sat_calls++;
  stats().sat_time += clock() - t1;
  return result;
}

uint Solver::NumOfModels() {
  auto t1 = clock();
  auto result = _NumOfModels();
  stats().models_calls++;
  stats().models_time += clock() - t1;
  return result;
}

vec<vec<bool>> Solver::GenerateModels() {
  auto t1 = clock();
  auto result = _GenerateModels();
  stats().models_calls++;
  stats().models_time += clock() - t1;
  return result;
}


// Adding general constraints in CnfSolver

void CnfSolver::AddConstraint(Formula* formula) {
  assert(formula);
  formula->ResetTseitinIds();
  formula->TseitinTransformation(*this, true);
}

void CnfSolver::AddConstraint(Formula* formula, const vec<CharId>& params) {
  build_for_params_ = &params;
  AddConstraint(formula);
  build_for_params_ = nullptr;
}

bool CnfSolver::_OnlyOneModel() {
  vec<bool> ass = GetModel();
  OpenContext();

  vec<VarId> clause;
  for (uint id = 1; id < var_count_; id++) {
    clause.push_back(id * (ass[id] ? -1 : 1));
  }
  AddClause(clause);

  auto s = _Satisfiable();
  CloseContext();
  return !s;
}

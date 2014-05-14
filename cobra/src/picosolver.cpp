/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "./picosolver.h"

extern "C" {
  #include <picosat/picosat.h>
}

#include <cstdio>
#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "./formula.h"
#include "./common.h"

SolverStats PicoSolver::stats_ = SolverStats();

PicoSolver::PicoSolver(uint var_count, Formula* restriction) {
  var_count_ = var_count;
  picosat_ = picosat_init();
  // Reserve id's for original variables.
  for (uint i = 1; i < var_count_; i++) {
    picosat_inc_max_var(picosat_);
    picosat_set_more_important_lit(picosat_, i);
  }
  if (restriction) AddConstraint(restriction);
}

PicoSolver::~PicoSolver() {
  picosat_reset(picosat_);
}

//------------------------------------------------------------------------------
// Adding constraints

void PicoSolver::AddClause(const vec<VarId>& list) {
  for (auto l : list) {
    assert(l != 0);
    picosat_add(picosat_, l);
  }
  picosat_add(picosat_, 0);
}

void PicoSolver::AddClause(std::initializer_list<VarId> list) {
  for (auto l : list) {
    assert(l != 0);
    picosat_add(picosat_, l);
  }
  picosat_add(picosat_, 0);
}

//------------------------------------------------------------------------------

void PicoSolver::OpenContext() {
  picosat_push(picosat_);
}

void PicoSolver::CloseContext() {
  picosat_pop(picosat_);
}


//------------------------------------------------------------------------------
// SAT solver stuff

bool PicoSolver::_MustBeTrue(VarId id) {
  assert(id > 0);
  picosat_assume(picosat_, -id);
  return !_Satisfiable();
}

bool PicoSolver::_MustBeFalse(VarId id) {
  assert(id > 0);
  picosat_assume(picosat_, id);
  return !_Satisfiable();
}

vec<VarId> PicoSolver::_GetFixedVars() {
  vec<VarId> result;
  for (uint id = 1; id < var_count_; id++) {
    if (_MustBeTrue(id)) result.push_back(id);
    if (_MustBeFalse(id)) result.push_back(-id);
  }
  return result;
}

uint PicoSolver::_GetNumOfFixedVars() {
  uint r = 0;
  for (uint id = 1; id < var_count_; id++) {
    r += _MustBeTrue(id);
    r += _MustBeFalse(id);
  }
  return r;
}

bool PicoSolver::_Satisfiable() {
  auto result = picosat_sat(picosat_, -1);
  return (result == PICOSAT_SATISFIABLE);
}

vec<bool> PicoSolver::GetModel() {
  vec<bool> result(var_count_, false);
  for (uint id = 1; id < var_count_; id++) {
    if (picosat_deref(picosat_, id) == 1) result[id] = true;
  }
  return result;
}

// uint PicoSolver::NumOfModelsSharpSat(){
//   FILE* f = fopen(".nummodels", "w");
//   WriteDimacs(f);
//   fclose(f);
//   int r = system("./tools/sharpSAT/Release/sharpSAT -q"
//        " .nummodels > .nummodels-result");
//   if (r != 0) {
//     printf("Error: sharpSAT failed :-(\n");
//     return 0;
//   }
//   FILE* g = fopen(".nummodels-result", "r");
//   uint k;
//   fscanf(g, "%u", &k);
//   return k;
// }

void PicoSolver::ForAllModels(VarId var, std::function<void()> callback) {
  assert(var > 0 && (unsigned)var < var_count_);
  for (VarId v : std::initializer_list<VarId>({var, -var})) {
    picosat_assume(picosat_, v);
    if (picosat_sat(picosat_, -1) == PICOSAT_SATISFIABLE) {
      if ((unsigned)var == var_count_ - 1) {
        callback();
      } else {
        picosat_push(picosat_);
        picosat_add(picosat_, v);
        picosat_add(picosat_, 0);
        ForAllModels(var + 1, callback);
        picosat_pop(picosat_);
      }
    }
  }
}

uint PicoSolver::_NumOfModels() {
  uint k = 0;
  ForAllModels(1, [&]() { k++; });
  return k;
}

vec<vec<bool>> PicoSolver::_GenerateModels() {
  vec<vec<bool>> models;
  ForAllModels(1, [&]() {
    vec<bool> n(var_count_, false);
    for (uint id = 1; id < var_count_; id++) {
      if (picosat_deref(picosat_, id) == 1) n[id] = true;
    }
    models.push_back(n);
  });
  return models;
}

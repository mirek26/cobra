/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "common.h"

#ifndef COBRA_SOLVER_H_
#define COBRA_SOLVER_H_

class Variable;
class Formula;
class Solver;

typedef struct SolverStats {
  clock_t fixed_time = 0;
  int fixed_calls = 0;
  clock_t sat_time = 0;
  int sat_calls = 0;
  clock_t models_time = 0;
  int models_calls = 0;
} SolverStats;

class Solver {
 public:
  virtual ~Solver() { }
  virtual SolverStats& stats() = 0;

  virtual void AddConstraint(Formula* formula) = 0;
  virtual void AddConstraint(Formula* formula, const vec<CharId>& params) = 0;

  virtual void OpenContext() = 0;
  virtual void CloseContext() = 0;

  bool MustBeTrue(VarId id) {
    auto t1 = clock();
    auto result = _MustBeTrue(id);
    stats().fixed_calls++;
    stats().fixed_time += clock() - t1;
    return result;
  }

  bool MustBeFalse(VarId id) {
    auto t1 = clock();
    auto result = _MustBeFalse(id);
    stats().fixed_calls++;
    stats().fixed_time += clock() - t1;
    return result;
  }

  vec<VarId> GetFixedVars() {
    auto t1 = clock();
    auto result = _GetFixedVars();
    stats().fixed_calls++;
    stats().fixed_time += clock() - t1;
    return result;
  }

  uint GetNumOfFixedVars() {
    auto t1 = clock();
    auto result = _GetNumOfFixedVars();
    stats().fixed_calls++;
    stats().fixed_time += clock() - t1;
    return result;
  }

  bool Satisfiable() {
    auto t1 = clock();
    auto result = _Satisfiable();
    stats().sat_calls++;
    stats().sat_time += clock() - t1;
    return result;
  }

  bool OnlyOneModel() {
    // should be called right after Satisfiable
    auto t1 = clock();
    auto result = _OnlyOneModel();
    stats().sat_calls++;
    stats().sat_time += clock() - t1;
    return result;
  }

  uint NumOfModels() {
    auto t1 = clock();
    auto result = _NumOfModels();
    stats().models_calls++;
    stats().models_time += clock() - t1;
    return result;
  }

  vec<vec<bool>> GenerateModels() {
    auto t1 = clock();
    auto result = _GenerateModels();
    stats().models_calls++;
    stats().models_time += clock() - t1;
    return result;
  }

  virtual vec<bool> GetAssignment() = 0;
  virtual void PrintAssignment() = 0;
  virtual string pretty() = 0;

 private:

  virtual bool _MustBeTrue(VarId id) = 0;
  virtual bool _MustBeFalse(VarId id) = 0;
  virtual vec<VarId> _GetFixedVars() = 0;
  virtual uint _GetNumOfFixedVars() = 0;
  virtual bool _Satisfiable() = 0;
  virtual bool _OnlyOneModel() = 0;
  virtual uint _NumOfModels() = 0;
  virtual vec<vec<bool>> _GenerateModels() = 0;

};

#endif   // COBRA_SOLVER_H_

/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cassert>
#include <vector>
#include <map>
#include <utility>
#include <string>
#include <set>
#include "./common.h"
#include "./solver.h"

#ifndef COBRA_SRC_SIMPLE_SOLVER_H_
#define COBRA_SRC_SIMPLE_SOLVER_H_

class Variable;
class Formula;

class SimpleSolver: public Solver {
  static SolverStats stats_;

  Formula* constraint_;

  vec<std::pair<Formula*, vec<CharId>>> constraints_;
  vec<int> contexts_;

  vec<vec<bool>> codes_;
  vec<vec<uint>> context_unsat_;
  vec<uint> sat_;
  bool ready_;

 public:
  SimpleSolver(uint var_count, Formula* constraint = nullptr);

  SolverStats& stats() { return stats_; }
  static SolverStats& s_stats() { return stats_; }

  void AddConstraint(Formula* formula);
  void AddConstraint(Formula* formula, const vec<CharId>& params);

  void OpenContext();
  void CloseContext();

  bool _MustBeTrue(VarId id);
  bool _MustBeFalse(VarId id);
  vec<VarId> _GetFixedVars();
  uint _GetNumOfFixedVars();

  bool _Satisfiable();
  bool _OnlyOneModel();

  vec<bool> GetModel();

  uint _NumOfModels();
  vec<vec<bool>> _GenerateModels();

  string pretty();

 private:
  bool TestSat(uint i);
  void RemoveUntilSat(uint start);
  void Update();
};

#endif  // COBRA_SRC_SIMPLE_SOLVER_H_

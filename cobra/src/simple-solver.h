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
#include "solver.h"

#ifndef COBRA_SOLVER_SIMPLE_H_
#define COBRA_SOLVER_SIMPLE_H_

class Variable;
class Formula;

class SimpleSolver: public Solver {
  static SolverStats stats_;

  const vec<Variable*>& vars_;
  Formula* restriction_;

  vec<std::pair<Formula*, vec<CharId>>> constraints_;
  vec<int> contexts_;

  vec<vec<bool>> codes_;
  vec<vec<uint>> context_unsat_;
  vec<uint> sat_;
  bool ready_;

 public:
  SimpleSolver(const vec<Variable*>& vars, Formula* restriction = nullptr);

  virtual SolverStats& stats() { return stats_; }
  static SolverStats& s_stats() { return stats_; }

  virtual void AddConstraint(Formula* formula);
  virtual void AddConstraint(Formula* formula, const vec<CharId>& params);

  virtual void OpenContext();
  virtual void CloseContext();

  virtual bool _MustBeTrue(VarId id);
  virtual bool _MustBeFalse(VarId id);
  virtual vec<VarId> _GetFixedVars();
  virtual uint _GetNumOfFixedVars();

  virtual bool _Satisfiable();
  virtual bool _OnlyOneModel();

  virtual vec<bool> GetAssignment();
  virtual void PrintAssignment();

  virtual uint _NumOfModels();
  virtual vec<vec<bool>> _GenerateModels();

  virtual string pretty();

 private:

  bool TestSat(uint i);
  void RemoveUntilSat(uint start);
  void Update();
};

#endif   // COBRA_SOLVER_SIMPLE_H_

/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include <minisat/core/Solver.h>
#include "./common.h"
#include "./solver.h"

#ifndef COBRA_SRC_MINISOLVER_H_
#define COBRA_SRC_MINISOLVER_H_

class Variable;
class Formula;

typedef set<VarId> Clause;

class MiniSolver: public CnfSolver {
  static SolverStats stats_;

  Minisat::Solver minisat_;
  Minisat::vec<Minisat::Lit> contexts_;

 public:
  MiniSolver(uint var_count, Formula* constraint = nullptr);
  ~MiniSolver() { }

  SolverStats& stats() { return stats_; }
  static SolverStats& s_stats() { return stats_; }

  void AddClause(const vec<VarId>& list);
  void AddClause(std::initializer_list<VarId> list);

  void OpenContext();
  void CloseContext();

  // void WriteDimacs(FILE* f) {
  //   minisat_print(minisat_, f);
  // }

  VarId NewVarId() {
    return minisat_.newVar(true, false) + 1;
    // assert(minisat_);
    // int k = minisat_inc_max_var(minisat_);
    // return k;
  }

  vec<bool> GetModel();

  // uint NumOfModelsSharpSat();

 private:
  bool _MustBeTrue(VarId id);
  bool _MustBeFalse(VarId id);
  vec<VarId> _GetFixedVars();
  uint _GetNumOfFixedVars();
  bool _Satisfiable();
  uint _NumOfModels();
  vec<vec<bool>> _GenerateModels();

  void ForAllModels(VarId var, std::function<void()> callback);
};

#endif  // COBRA_SRC_MINISOLVER_H_

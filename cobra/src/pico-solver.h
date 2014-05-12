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

extern "C" {
  #include <picosat/picosat.h>
}

#ifndef COBRA_PICOSOLVER_H_
#define COBRA_PICOSOLVER_H_

class Variable;
class Formula;

typedef set<VarId> Clause;

class PicoSolver: public CnfSolver {
  static SolverStats stats_;

  vec<Clause> clauses_;
  PicoSAT* picosat_;
  vec<int> context_;
  const vec<Variable*>& vars_;

 public:
  PicoSolver(const vec<Variable*>& vars, Formula* restriction = nullptr);
  ~PicoSolver();

  SolverStats& stats() { return stats_; }
  static SolverStats& s_stats() { return stats_; }

  void AddClause(vec<VarId>& list);
  void AddClause(std::initializer_list<VarId> list);
  void AddClause(const set<VarId>& c);

  void OpenContext();
  void CloseContext();

  void WriteDimacs(FILE* f) {
    picosat_print(picosat_, f);
  }

  VarId NewVarId() {
    assert(picosat_);
    int k = picosat_inc_max_var(picosat_);
    return k;
  }

  vec<bool> GetAssignment();
  void PrintAssignment();

  // uint NumOfModelsSharpSat();

  string pretty();

 private:
  bool _MustBeTrue(VarId id);
  bool _MustBeFalse(VarId id);
  vec<VarId> _GetFixedVars();
  uint _GetNumOfFixedVars();
  bool _Satisfiable();
  bool _OnlyOneModel();
  uint _NumOfModels();
  vec<vec<bool>> _GenerateModels();

  string pretty_clause(const Clause& clause);

  void ForAllModels(VarId var, std::function<void()> callback);
};

#endif   // COBRA_PICOSOLVER_H_

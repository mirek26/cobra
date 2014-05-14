/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

extern "C" {
  #include <picosat/picosat.h>
}

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "./common.h"
#include "./solver.h"

#ifndef COBRA_SRC_PICOSOLVER_H_
#define COBRA_SRC_PICOSOLVER_H_

class Variable;
class Formula;

class PicoSolver: public CnfSolver {
  static SolverStats stats_;
  PicoSAT* picosat_;

 public:
  PicoSolver(uint var_count, Formula* restriction = nullptr);
  ~PicoSolver();

  SolverStats& stats() { return stats_; }
  static SolverStats& s_stats() { return stats_; }

  void AddClause(const vec<VarId>& list);
  void AddClause(std::initializer_list<VarId> list);

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

#endif  // COBRA_SRC_PICOSOLVER_H_

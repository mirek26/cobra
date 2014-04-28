/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
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

#ifndef COBRA_CNF_FORMULA_H_
#define COBRA_CNF_FORMULA_H_

class Variable;
class Formula;

typedef set<VarId> Clause;

class PicoSolver: public Solver {
  static SolverStats stats_;

  vec<Clause> clauses_;
  PicoSAT* picosat_;
  vec<int> context_;
  const vec<Variable*>& vars_;
  const vec<CharId>* build_for_params_ = nullptr;

 public:
  PicoSolver(const vec<Variable*>& vars, Formula* restriction = nullptr);
  ~PicoSolver();

  virtual SolverStats& stats() { return stats_; }
  static SolverStats& s_stats() { return stats_; }

  const vec<CharId>* build_for_params() const { return build_for_params_; }

  void AddClause(vec<VarId>& list);
  void AddClause(std::initializer_list<VarId> list);
  void AddClause(const set<VarId>& c);
  void AddConstraint(Formula* formula);
  void AddConstraint(Formula* formula, const vec<CharId>& params);
  void AddConstraint(PicoSolver& cnf);

  virtual void OpenContext();
  virtual void CloseContext();

  void WriteDimacs(FILE* f) {
    picosat_print(picosat_, f);
  }

  VarId NewVarId() {
    assert(picosat_);
    int k = picosat_inc_max_var(picosat_);
    return k;
  }

  virtual vec<bool> GetAssignment();
  virtual void PrintAssignment();

  uint NumOfModelsSharpSat();

  string pretty();

 private:
  virtual bool _MustBeTrue(VarId id);
  virtual bool _MustBeFalse(VarId id);
  virtual uint _GetNumOfFixedVars();
  virtual bool _Satisfiable();
  virtual bool _OnlyOneModel();
  virtual uint _NumOfModels();
  virtual vec<vec<bool>> _GenerateModels();

  string pretty_clause(const Clause& clause);

  void ForAllModels(VarId var, std::function<void()> callback);
  //bool ProbeEquivalence(const Clause& clause, VarId var1, VarId var2);

};

#endif   // COBRA_CNF_FORMULA_H_

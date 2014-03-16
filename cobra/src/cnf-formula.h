/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "common.h"

extern "C" {
  #include <picosat/picosat.h>
}

#ifndef COBRA_CNF_FORMULA_H_
#define COBRA_CNF_FORMULA_H_

class Variable;
class Formula;

typedef set<VarId> Clause;

class CnfFormula {
  set<Clause> clauses_;
  PicoSAT* picosat_;

  vec<CharId>* build_for_params_ = nullptr;

 private:
  string pretty_clause(const Clause& clause);

  bool ProbeEquivalence(const Clause& clause, VarId var1, VarId var2);

 public:

  vec<CharId>* build_for_params() { return build_for_params_; }
  void set_build_for_params(vec<CharId>* value) {
    build_for_params_ = value;
  }

  void addClause(vec<VarId>& list);
  void addClause(std::initializer_list<VarId> list);
  void AddConstraint(Formula* formula);
  void AddConstraint(Formula* formula, vec<CharId> params);
  void AddConstraint(CnfFormula& cnf);

  uint GetFixedVariables();
  uint GetFixedPairs();
  bool HasOnlyOneModel() {
    // assert(Satisfiable());
    // return GetFixedVariables() == original_.size();
    return false;
  }

  bool Satisfiable();

  void WriteDimacs(FILE* f) {
    picosat_print(picosat_, f);
  }

//  void ResetPicosat();

  void InitSolver();
  void PrintAssignment(vec<Variable*>& vars);

  vec<int> ComputeVariableEquivalence(VarId limit);

  string pretty();
};

#endif   // COBRA_CNF_FORMULA_H_

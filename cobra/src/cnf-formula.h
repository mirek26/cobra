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
  VarId limit_;
  vec<Clause> clauses_;
  PicoSAT* picosat_;
  vec<int> context_;

  const vec<CharId>* build_for_params_ = nullptr;

 public:
  CnfFormula(VarId limit);

  const vec<CharId>* build_for_params() const { return build_for_params_; }
  void set_build_for_params(vec<CharId>* value) {
    build_for_params_ = value;
  }

  void AddClause(vec<VarId>& list);
  void AddClause(std::initializer_list<VarId> list);
  void AddClause(const set<VarId>& c);
  void AddConstraint(Formula* formula);
  void AddConstraint(Formula* formula, const vec<CharId>& params);
  void AddConstraint(CnfFormula& cnf);

  void OpenContext();
  void CloseContext();

  bool MustBeTrue(VarId id);
  bool MustBeFalse(VarId id);
  uint GetNumOfFixedVars();

  bool Satisfiable();

  void WriteDimacs(FILE* f) {
    picosat_print(picosat_, f);
  }

  VarId NewVarId() {
    return picosat_inc_max_var(picosat_);
  }

  void PrintAssignment(vec<Variable*>& vars);

  //vec<int> ComputeVariableEquivalence(VarId limit);

  uint NumOfModelsSharpSat();
  uint NumOfModels();

  string pretty();

 private:
  string pretty_clause(const Clause& clause);

  //bool ProbeEquivalence(const Clause& clause, VarId var1, VarId var2);
  void NumOfModelsRecursive(VarId s, uint* num);

};

#endif   // COBRA_CNF_FORMULA_H_

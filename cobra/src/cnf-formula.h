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

typedef std::set<VarId> Clause;

class CnfFormula {
  std::set<Clause> clauses_;
  PicoSAT* picosat_;

  std::vector<CharId>* build_for_params_ = nullptr;

 private:
  std::string pretty_clause(const Clause& clause);

  bool ProbeEquivalence(const Clause& clause, VarId var1, VarId var2);

 public:

  std::vector<CharId>* build_for_params() { return build_for_params_; }
  void set_build_for_params(std::vector<CharId>* value) {
    build_for_params_ = value;
  }

  void addClause(std::vector<VarId>& list);
  void addClause(std::initializer_list<VarId> list);
  void AddConstraint(Formula* formula);
  void AddConstraint(Formula* formula, std::vector<CharId> params);
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
  void PrintAssignment(std::vector<Variable*>& vars);

  std::vector<int> ComputeVariableEquivalence(VarId limit);

  std::string pretty();
};

#endif   // COBRA_CNF_FORMULA_H_

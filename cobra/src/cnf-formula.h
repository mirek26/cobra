/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "common.h"

extern "C" {
  #include "../sat-solvers/picosat/picosat.h"
}

#ifndef COBRA_CNF_FORMULA_H_
#define COBRA_CNF_FORMULA_H_

class Variable;
class Formula;

typedef std::set<VarId> Clause;

class CnfFormula {
  std::set<Clause> clauses_;
  std::map<int, Variable*> variables_;
  std::set<int> original_;
  PicoSAT* picosat_;

  std::vector<CharId>* build_for_params_ = nullptr;
//  std::vector<ParameterEq*> paramEq_;

 private:
  Variable* getVariable(int id);
  void addVariable(Variable* var);
  void addLiteral(Clause& clause, Formula* literal);

  std::string pretty_literal(int id, bool unicode = true);
  std::string pretty_clause(const Clause& clause, bool unicode = true);

  bool ProbeEquivalence(const Clause& clause, int var1, int var2);

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


  //CnfFormula SubstituteParams(std::vector<Variable*> params);

  uint GetFixedVariables();
  uint GetFixedPairs();
  bool HasOnlyOneModel() {
    // assert(Satisfiable());
    // return GetFixedVariables() == original_.size();
    return false;
  }

  bool Satisfiable();

//  void ResetPicosat();

  void InitSolver();
  void PrintAssignment(int limit);

  std::vector<int> ComputeVariableEquivalence(int);

  std::string pretty(bool unicode = true);
};

#endif   // COBRA_CNF_FORMULA_H_

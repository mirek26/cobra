/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include <set>

extern "C" {
  #include "sat-solvers/picosat/picosat.h"
}

#ifndef COBRA_CNF_FORMULA_H_
#define COBRA_CNF_FORMULA_H_

class Variable;
class Formula;
class FormulaList;

class CnfFormula {
  std::vector<std::vector<int>> clauses_;
  std::map<int, Variable*> variables_;
  std::set<int> original_;
  PicoSAT* picosat_;

 private:
  Variable* getVariable(int id);
  void addVariable(Variable* var);
  void addLiteral(Formula* literal);

  std::string pretty_literal(int id, bool unicode = true);
  std::string pretty_clause(std::vector<int>& clause, bool unicode);

 public:
  CnfFormula() {
    picosat_ = picosat_init();
  }

  void addClause(FormulaList* list);
  void addClause(std::initializer_list<Formula*> list);
  void AddConstraint(Formula* formula);
  void AddConstraint(CnfFormula& cnf);

  int GetFixedVariables();
  bool Satisfiable();
  void PrintAssignment();

  std::string pretty(bool unicode = true);
};

#endif   // COBRA_CNF_FORMULA_H_

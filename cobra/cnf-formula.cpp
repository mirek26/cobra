/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <map>

#include "formula.h"
#include "util.h"

extern "C" {
  #include "sat-solvers/picosat/picosat.h"
}

//------------------------------------------------------------------------------
// Adding constraints

Variable* CnfFormula::getVariable(int id) {
  return (variables_.count(id) > 0 ? variables_[id] : nullptr);
}

// adds variable to variables_ and original_
void CnfFormula::addVariable(Variable* var) {
  assert(var);
  variables_[var->id()] = var;
  if (!var->generated()) {
    original_.insert(var->id());
  }
}

void CnfFormula::addLiteral(Formula* literal) {
  assert(literal->isLiteral());
  auto var = dynamic_cast<Variable*>(literal);
  auto neg = false;
  if (!var) {
    var = dynamic_cast<Variable*>(literal->neg());
    neg = true;
  }
  addVariable(var);
  // add to the last clause
  int add = neg ? -var->id(): var->id();
  clauses_.back().push_back(add);
  picosat_add(picosat_, add);
}

void CnfFormula::addClause(FormulaList* list) {
  clauses_.push_back(std::vector<int>());
  for (auto f: *list) {
    addLiteral(f);
  }
  picosat_add(picosat_, 0);
}

void CnfFormula::addClause(std::initializer_list<Formula*> list) {
  clauses_.push_back(std::vector<int>());
  for (auto f: list) {
    addLiteral(f);
  }
  picosat_add(picosat_, 0);
}

void CnfFormula::AddConstraint(Formula* formula) {
  CnfFormula* cnf = formula->ToCnf();
  AddConstraint(*cnf);
  delete cnf;
}

void CnfFormula::AddConstraint(CnfFormula& cnf) {
  clauses_.insert(clauses_.end(),
                  cnf.clauses_.begin(),
                  cnf.clauses_.end());
  for (auto& clause: cnf.clauses_) {
    for (auto lit: clause) {
      picosat_add(picosat_, lit);
      if (variables_.count(abs(lit)) == 0) {
        addVariable(cnf.getVariable(abs(lit)));
      }
    }
    picosat_add(picosat_, 0);
  }
}

//------------------------------------------------------------------------------
// Pretty print

std::string CnfFormula::pretty_literal(int id, bool unicode) {
  return (id > 0 ? variables_[id] : variables_[-id]->neg())->pretty(unicode);
}

std::string CnfFormula::pretty_clause(std::vector<int>& clause, bool unicode) {
  std::string s = "(" + pretty_literal(clause.front(), unicode);
  for (auto it = std::next(clause.begin()); it != clause.end(); ++it) {
    s += unicode ? "|" : "|";
    s += pretty_literal(*it, unicode);
  }
  s += ")";
  return s;
}

std::string CnfFormula::pretty(bool unicode) {
  std::string s = pretty_clause(clauses_.front(), unicode);
  for (auto it = std::next(clauses_.begin()); it != clauses_.end(); ++it) {
    s += unicode ? " & " : " & ";
    s += pretty_clause(*it, unicode);
  }
  return s;
}

//------------------------------------------------------------------------------
// SAT solver stuff

int CnfFormula::GetFixedVariables() {
  int r = 0;
  for (auto& var: original_) {
    picosat_assume(picosat_, var);
    if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) r++;
    picosat_assume(picosat_, -var);
    if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) r++;
  }
  return r;
}

bool CnfFormula::Satisfiable() {
  auto result = picosat_sat(picosat_, -1);
  return (result == PICOSAT_SATISFIABLE);
}

void CnfFormula::PrintAssignment() {
  std::vector<int> trueVar;
  std::vector<int> falseVar;
  for (auto id: original_) {
    if (picosat_deref(picosat_, id) == 1) trueVar.push_back(id);
    else falseVar.push_back(id);
  }
  printf("TRUE: ");
  for (auto s: trueVar) printf("%s ", pretty_literal(s).c_str());
  printf("\nFALSE: ");
  for (auto s: falseVar) printf("%s ", pretty_literal(s).c_str());
  printf("\n");
}
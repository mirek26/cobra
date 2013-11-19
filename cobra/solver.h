/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <set>
#include "formula.h"
#include "util.h"

extern "C" {
  #include "sat-solvers/picosat/picosat.h"
}

#ifndef COBRA_SOLVER_H_
#define COBRA_SOLVER_H_

class Solver
{
  AndOperator* formula_;
  PicoSAT* picosat_;
  std::set<Variable*> variables_;

 public:
  Solver() {
    formula_ = m.get<AndOperator>();
    picosat_ = picosat_init();
  }

  AndOperator* formula() {
    return formula_;
  }

  int GetFixedVariables(VariableSet* vars) {
    int r = 0;
    for (auto& var: variables_) {
      // printf("Fixed test %i - %s\n", var->id(), var->pretty().c_str());
      picosat_assume(picosat_, var->id());
      if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) {
        printf("%s must be FALSE.\n", var->pretty().c_str());
        r++;
      }
      picosat_assume(picosat_, -var->id());
      if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) {
        printf("%s must be TRUE.\n", var->pretty().c_str());
        r++;
      }
    }
    return r;
  }

  bool Satisfiable() {
    auto result = picosat_sat(picosat_, -1);
    return (result == PICOSAT_SATISFIABLE);
  }

  void AddConstraint(Formula* constraint) {
    auto cnf = constraint->ToCnf();
    formula_->addChildren(cnf->children());
    for (auto clause: *cnf->children()) {
      auto orOperator = dynamic_cast<OrOperator*>(clause);
      assert(orOperator);
      for (auto& literal: *orOperator->children()) {
        auto neg = dynamic_cast<NotOperator*>(literal);
        auto var = dynamic_cast<Variable*>(neg ? neg->child(0) : literal);
        assert(var);
        variables_.insert(var);
        picosat_add(picosat_, (neg ? -1 : 1) * var->id());
      }
      picosat_add(picosat_, 0);
    }
  }

  void PrintAssignment() {
    std::vector<std::string> trueVar;
    std::vector<std::string> falseVar;
    for (auto& var: variables_) {
      if (picosat_deref(picosat_, var->id()) == 1) {
        trueVar.push_back(var->pretty());
      } else {
        falseVar.push_back(var->pretty());
      }
    }
    printf("TRUE: ");
    for (auto& s: trueVar) printf("%s ", s.c_str());
    printf("\nFALSE: ");
    for (auto& s: falseVar) printf("%s ", s.c_str());
    printf("\n");
  }
};
#endif   // COBRA_SOLVER_H_
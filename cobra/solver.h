/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
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

 public:
  Solver() {
    picosat_ = picosat_init();
  }

  AndOperator* formula() {
    return formula_;
  }

  bool Satisfiable() {
    auto result = picosat_sat(picosat_, -1);
    return (result == PICOSAT_SATISFIABLE);
  }

  void AddConstraint(Formula* constraint) {
    auto cnf = constraint->ToCnf();
    formula_->addChildren(cnf->children());
    for (auto& clause: cnf->children()) {
      picosat_add(picosat_, 0);
      auto orOperator = dynamic_cast<OrOperator*>(clause);
      assert(orOperator);
      for (auto& literal: orOperator->children()) {
        auto neg = dynamic_cast<NotOperator*>(literal);
        auto var = dynamic_cast<Variable*>(neg ? neg->child(0) : literal);
        assert(var);
        picosat_add(picosat_, (neg ? -1 : 1) * var->id());
      }
    }
  }
};
#endif   // COBRA_SOLVER_H_
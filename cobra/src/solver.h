/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "common.h"

#ifndef COBRA_SOLVER_H_
#define COBRA_SOLVER_H_

class Variable;
class Formula;

typedef set<VarId> Clause;

class Solver {

 public:
  //Solver(const vec<Variable*>& vars, Formula* restriction) { };

  virtual void AddConstraint(Formula* formula) = 0;
  virtual void AddConstraint(Formula* formula, const vec<CharId>& params) = 0;

  virtual void OpenContext() = 0;
  virtual void CloseContext() = 0;

  virtual bool MustBeTrue(VarId id) = 0;
  virtual bool MustBeFalse(VarId id) = 0;
  virtual uint GetNumOfFixedVars() = 0;

  virtual bool Satisfiable() = 0;
  virtual void PrintAssignment() = 0;
  virtual uint NumOfModels() = 0;

  virtual string pretty() = 0;

};

#endif   // COBRA_SOLVER_H_

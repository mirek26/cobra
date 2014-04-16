/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "common.h"
#include "solver.h"
#include "cnf-formula.h"

#ifndef COBRA_SOLVER_SIMPLE_H_
#define COBRA_SOLVER_SIMPLE_H_

class Variable;
class Formula;

class SimpleSolver: public Solver {
  vec<std::pair<Formula*, vec<CharId>>> constraints_;
  vec<int> contexts_;

  const vec<Variable*>& vars_;
  Formula* restriction_;
  vec<vec<bool>> codes_;
  vec<uint> sat_;

  bool ready_;

 public:
  SimpleSolver(const vec<Variable*>& vars, Formula* restriction) :
      vars_(vars), restriction_(restriction) {
    CnfFormula sat(vars, restriction);
    codes_ = sat.GenerateModels();
    for (uint i = 0; i < codes_.size(); i++)
      sat_.push_back(i);
    ready_ = true;
  }

  virtual void AddConstraint(Formula* formula) {
    ready_ = false;
    constraints_.push_back(make_pair(formula, vec<CharId>()));
  }

  virtual void AddConstraint(Formula* formula, const vec<CharId>& params) {
    ready_ = false;
    constraints_.push_back(make_pair(formula, params));
  }

  virtual void OpenContext() {
    contexts_.push_back(constraints_.size());
  }

  virtual void CloseContext() {
    auto k = contexts_.back();
    constraints_.erase(constraints_.begin() + k, constraints_.end());
    contexts_.pop_back();
    ready_ = false;
  }

  virtual bool MustBeTrue(VarId id) {
    if (!ready_) Update();
    for (auto& x: sat_)
      if (!codes_[x][id]) return false;
    return true;
  }

  virtual bool MustBeFalse(VarId id) {
    if (!ready_) Update();
    for (auto& x: sat_)
      if (codes_[x][id]) return false;
    return true;
  }

  virtual uint GetNumOfFixedVars() {
    if (!ready_) Update();
    vec<bool> canbe[2];
    canbe[0].resize(vars_.size());
    canbe[1].resize(vars_.size());
    for (auto x: sat_)
      for (uint i = 0; i < vars_.size(); i++)
        canbe[codes_[x][i]][i] = true;
    uint f = 0;
    for (uint i = 0; i < vars_.size(); i++)
      if (!(canbe[0][i] && canbe[1][i])) f++;
    return f;
  }

  virtual bool Satisfiable() {
    if (!ready_) Update();
    return !sat_.empty();
  }

  virtual void PrintAssignment();

  virtual uint NumOfModels() {
    if (!ready_) Update();
    return sat_.size();
  }

  virtual string pretty();

 private:

  void Update();

  //string pretty_clause(const Clause& clause);

  //bool ProbeEquivalence(const Clause& clause, VarId var1, VarId var2);
  //void NumOfModelsRecursive(VarId s, uint* num);

};

#endif   // COBRA_SOLVER_SIMPLE_H_

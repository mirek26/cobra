/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "common.h"
#include "solver.h"

extern "C" {
  #include <picosat/picosat.h>
}

#ifndef COBRA_CNF_FORMULA_H_
#define COBRA_CNF_FORMULA_H_

class Variable;
class Formula;

class CnfFormula: public Solver {
  vec<Clause> clauses_;
  PicoSAT* picosat_;
  vec<int> context_;
  const vec<Variable*>& vars_;
  const vec<CharId>* build_for_params_ = nullptr;

 public:
  CnfFormula(const vec<Variable*>& vars, Formula* restriction);

  const vec<CharId>* build_for_params() const { return build_for_params_; }

  void AddClause(vec<VarId>& list);
  void AddClause(std::initializer_list<VarId> list);
  void AddClause(const set<VarId>& c);
  void AddConstraint(Formula* formula);
  void AddConstraint(Formula* formula, const vec<CharId>& params);
  void AddConstraint(CnfFormula& cnf);

  virtual void OpenContext();
  virtual void CloseContext();

  virtual bool MustBeTrue(VarId id);
  virtual bool MustBeFalse(VarId id);
  virtual uint GetNumOfFixedVars();

  virtual bool Satisfiable();

  void WriteDimacs(FILE* f) {
    picosat_print(picosat_, f);
  }

  VarId NewVarId() {
    assert(picosat_);
    int k = picosat_inc_max_var(picosat_);
    return k;
  }

  virtual void PrintAssignment();

  virtual uint NumOfModelsSharpSat();
  virtual uint NumOfModels();
  vec<vec<bool>> GenerateModels();

  string pretty();

 private:
  string pretty_clause(const Clause& clause);

  void NumOfModelsRecursive(VarId var, std::function<void()> callback);
  //bool ProbeEquivalence(const Clause& clause, VarId var1, VarId var2);
  //void NumOfModelsRecursive(VarId s, uint* num);

};

#endif   // COBRA_CNF_FORMULA_H_

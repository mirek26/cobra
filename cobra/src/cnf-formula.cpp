/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "formula.h"
#include "common.h"

extern "C" {
  #include <picosat/picosat.h>
}

#include "cnf-formula.h"

CnfFormula::CnfFormula(VarId limit) {
  limit_ = limit;
  picosat_ = picosat_init();
  // Reserve id's for original variables.
  for (VarId i = 0; i < limit; i++)
    picosat_inc_max_var(picosat_);
}

//------------------------------------------------------------------------------
// Adding constraints

void CnfFormula::AddClause(vec<VarId>& list) {
  Clause c(list.begin(), list.end());
  AddClause(c);
}

void CnfFormula::AddClause(std::initializer_list<VarId> list) {
  Clause c(list.begin(), list.end());
  AddClause(c);
}

void CnfFormula::AddClause(const Clause& c) {
  clauses_.push_back(c);
  for (auto l: c) {
    assert(l != 0);
    picosat_add(picosat_, l);
  }
  picosat_add(picosat_, 0);
}

void CnfFormula::AddConstraint(Formula* formula) {
  assert(formula);
  formula->TseitinTransformation(*this, true);
}

void CnfFormula::AddConstraint(Formula* formula, const vec<CharId>& params) {
  assert(formula);
  build_for_params_ = &params;
  formula->TseitinTransformation(*this, true);
  build_for_params_ = nullptr;
}

void CnfFormula::AddConstraint(CnfFormula& cnf) {
  for (auto& clause: cnf.clauses_) {
    AddClause(clause);
  }
}

//------------------------------------------------------------------------------
// Pretty print

string CnfFormula::pretty_clause(const Clause& clause) {
  if (clause.empty()) return "()";
  string s = "(" + std::to_string(*clause.begin());
  for (auto it = std::next(clause.begin()); it != clause.end(); ++it) {
    s += " | " + std::to_string(*it);
  }
  s += ")";
  return s;
}

string CnfFormula::pretty() {
  if (clauses_.empty()) return "(empty)";
  string s = pretty_clause(*clauses_.begin());
  for (auto it = std::next(clauses_.begin()); it != clauses_.end(); ++it) {
    s += "&" + pretty_clause(*it);
  }
  return s;
}

//------------------------------------------------------------------------------

void CnfFormula::OpenContext() {
  picosat_push(picosat_);
  context_.push_back(clauses_.size());
}

void CnfFormula::CloseContext() {
  assert(!context_.empty());
  int k = context_.back();
  clauses_.erase(clauses_.begin()+k, clauses_.end());
  context_.pop_back();
  picosat_pop(picosat_);
}


//------------------------------------------------------------------------------
// SAT solver stuff

bool CnfFormula::MustBeTrue(VarId id) {
  assert(id > 0);
  picosat_assume(picosat_, -id);
  return !Satisfiable();
}

bool CnfFormula::MustBeFalse(VarId id) {
  assert(id > 0);
  picosat_assume(picosat_, id);
  return !Satisfiable();
}

uint CnfFormula::GetNumOfFixedVars() {
  uint r = 0;
  for (auto id = 1; id <= limit_; id++) {
    r += MustBeTrue(id);
    r += MustBeFalse(id);
  }
  return r;
}

bool CnfFormula::Satisfiable() {
  auto result = picosat_sat(picosat_, -1);
  return (result == PICOSAT_SATISFIABLE);
}

void CnfFormula::PrintAssignment(vec<Variable*>& vars) {
  vec<int> trueVar;
  vec<int> falseVar;
  for (uint id = 1; id <= vars.size(); id++) {
    if (picosat_deref(picosat_, id) == 1) trueVar.push_back(id);
    else falseVar.push_back(id);
  }
  printf("TRUE: ");
  for (auto s: trueVar) printf("%s ", vars[s-1]->ident().c_str());
  printf("\nFALSE: ");
  for (auto s: falseVar) printf("%s ", vars[s-1]->ident().c_str());
  printf("\n");
}

//------------------------------------------------------------------------------
// Symmetry-breaking stuff

// bool CnfFormula::ProbeEquivalence(const Clause& clause, VarId var1, VarId var2) {
//   Clause test(clause);
//   bool p1 = test.erase(var1);
//   bool p2 = test.erase(var2);
//   bool n1 = test.erase(-var1);
//   bool n2 = test.erase(-var2);
//   if (p1) test.insert(var2);
//   if (p2) test.insert(var1);
//   if (n1) test.insert(-var2);
//   if (n2) test.insert(-var1);
//   return clauses_.count(test);
// }

// // Compute 'syntactical' equivalence on variables with ids 1 - limit; how about semantical?
// vec<int> CnfFormula::ComputeVariableEquivalence(VarId limit) {
//   assert(context_.empty());
//   vec<int> components(limit + 1, 0);
//   int cid = 1;
//   for (VarId i = 1; i <= limit; i++) {
//     if (components[i] > 0) continue;
//     components[i] = cid++;
//     for (VarId j = i + 1; j <= limit; j++) {
//       if (components[j] > 0) continue;
//       bool equiv = true;
//       for (auto& clause: clauses_) {
//         equiv = equiv && ProbeEquivalence(clause, i, j);
//         if (!equiv) break;
//       }
//       if (equiv) components[j] = components[i];
//     }
//   }
//   return components;
// }

uint CnfFormula::NumOfModelsSharpSat(){
  FILE* f = fopen(".nummodels", "w");
  WriteDimacs(f);
  fclose(f);
  int r = system("./tools/sharpSAT/Release/sharpSAT -q .nummodels > .nummodels-result");
  if (r != 0) {
    printf("Error: sharpSAT failed :-(\n");
    return 0;
  }
  FILE* g = fopen(".nummodels-result", "r");
  uint k;
  fscanf(g, "%u", &k);
  return k;
}

void CnfFormula::NumOfModelsRecursive(VarId var, uint* num){
  for (VarId v: std::initializer_list<VarId>({var, -var})) {
    picosat_assume(picosat_, v);
    if (picosat_sat(picosat_, -1) == PICOSAT_SATISFIABLE) {
      if (var == limit_) (*num)++;
      else {
        picosat_push(picosat_);
        picosat_add(picosat_, v);
        picosat_add(picosat_, 0);
        NumOfModelsRecursive(var + 1, num);
        picosat_pop(picosat_);
      }
    }
  }
}

uint CnfFormula::NumOfModels() {
  uint k = 0;
  NumOfModelsRecursive(1, &k);
  return k;
}


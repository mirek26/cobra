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

#include "pico-solver.h"

SolverStats PicoSolver::stats_ = SolverStats();

PicoSolver::PicoSolver(const vec<Variable*>& vars, Formula* restriction):
    vars_(vars) {
  picosat_ = picosat_init();
  // Reserve id's for original variables.
  for (uint i = 1; i < vars_.size(); i++) {
    picosat_inc_max_var(picosat_);
    // TODO: try this
    //picosat_set_more_important_lit(picosat_, i);
  }
  // TODO: try this
  // picosat_set_global_default_phase(picosat_, false);
  if (restriction) AddConstraint(restriction);
}

//------------------------------------------------------------------------------
// Adding constraints

void PicoSolver::AddClause(vec<VarId>& list) {
  Clause c(list.begin(), list.end());
  AddClause(c);
}

void PicoSolver::AddClause(std::initializer_list<VarId> list) {
  Clause c(list.begin(), list.end());
  AddClause(c);
}

void PicoSolver::AddClause(const Clause& c) {
  clauses_.push_back(c);
  for (auto l: c) {
    assert(l != 0);
    picosat_add(picosat_, l);
  }
  picosat_add(picosat_, 0);
}

void PicoSolver::AddConstraint(Formula* formula) {
  assert(formula);
  formula->clearTseitinIds();
  formula->TseitinTransformation(*this, true);
}

void PicoSolver::AddConstraint(Formula* formula, const vec<CharId>& params) {
  build_for_params_ = &params;
  AddConstraint(formula);
  build_for_params_ = nullptr;
}

void PicoSolver::AddConstraint(PicoSolver& cnf) {
  for (auto& clause: cnf.clauses_) {
    AddClause(clause);
  }
}

//------------------------------------------------------------------------------
// Pretty print

string PicoSolver::pretty_clause(const Clause& clause) {
  if (clause.empty()) return "()";
  string s = "(";
  for (auto lit: clause) {
    int alit = abs(lit);
    s += (lit != alit ? "-" : "") +
         ((unsigned)alit < vars_.size() ? vars_[alit]->ident() : std::to_string(alit)) +
         " | ";
  }
  s.erase(s.size() - 3, 3);
  s += ")";
  return s;
}

string PicoSolver::pretty() {
  if (clauses_.empty()) return "()";
  string s = pretty_clause(*clauses_.begin());
  for (auto it = std::next(clauses_.begin()); it != clauses_.end(); ++it) {
    s += " & " + pretty_clause(*it);
  }
  return s;
}

//------------------------------------------------------------------------------

void PicoSolver::OpenContext() {
  picosat_push(picosat_);
  context_.push_back(clauses_.size());
}

void PicoSolver::CloseContext() {
  assert(!context_.empty());
  int k = context_.back();
  clauses_.erase(clauses_.begin()+k, clauses_.end());
  context_.pop_back();
  picosat_pop(picosat_);
}


//------------------------------------------------------------------------------
// SAT solver stuff

bool PicoSolver::_MustBeTrue(VarId id) {
  assert(id > 0);
  picosat_assume(picosat_, -id);
  return !_Satisfiable();
}

bool PicoSolver::_MustBeFalse(VarId id) {
  assert(id > 0);
  picosat_assume(picosat_, id);
  return !_Satisfiable();
}

uint PicoSolver::_GetNumOfFixedVars() {
  uint r = 0;
  for (uint id = 1; id < vars_.size(); id++) {
    r += _MustBeTrue(id);
    r += _MustBeFalse(id);
  }
  return r;
}

bool PicoSolver::_Satisfiable() {
  auto result = picosat_sat(picosat_, -1);
  return (result == PICOSAT_SATISFIABLE);
}

bool PicoSolver::_OnlyOneModel() {
  vec<bool> ass = GetAssignment();
  picosat_push(picosat_);
  for (uint id = 1; id < vars_.size(); id++) {
    picosat_add(picosat_, id * (ass[id] ? -1 : 1));
  }
  picosat_add(picosat_, 0);
  auto s = _Satisfiable();
  picosat_pop(picosat_);
  return !s;
}

vec<bool> PicoSolver::GetAssignment() {
  vec<bool> result(vars_.size(), false);
  for (uint id = 1; id < vars_.size(); id++) {
    if (picosat_deref(picosat_, id) == 1) result[id] = true;
  }
  return result;
}

void PicoSolver::PrintAssignment() {
  vec<int> trueVar;
  vec<int> falseVar;
  for (uint id = 1; id < vars_.size(); id++) {
    if (picosat_deref(picosat_, id) == 1) trueVar.push_back(id);
    else falseVar.push_back(id);
  }
  printf("TRUE: ");
  for (auto s: trueVar) printf("%s ", vars_[s]->ident().c_str());
  printf("\nFALSE: ");
  for (auto s: falseVar) printf("%s ", vars_[s]->ident().c_str());
  printf("\n");
}

//------------------------------------------------------------------------------
// Symmetry-breaking stuff

// bool PicoSolver::ProbeEquivalence(const Clause& clause, VarId var1, VarId var2) {
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
// vec<int> PicoSolver::ComputeVariableEquivalence(VarId limit) {
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

uint PicoSolver::NumOfModelsSharpSat(){
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

void PicoSolver::ForAllModels(VarId var, std::function<void()> callback){
  assert(var > 0 && (unsigned)var < vars_.size());
  for (VarId v: std::initializer_list<VarId>({var, -var})) {
    picosat_assume(picosat_, v);
    if (picosat_sat(picosat_, -1) == PICOSAT_SATISFIABLE) {
      if ((unsigned)var == vars_.size() - 1) {
        //PrintAssignment();
        callback();
      }
      else {
        picosat_push(picosat_);
        picosat_add(picosat_, v);
        picosat_add(picosat_, 0);
        ForAllModels(var + 1, callback);
        picosat_pop(picosat_);
      }
    }
  }
}

uint PicoSolver::_NumOfModels() {
  uint k = 0;
  ForAllModels(1, [&](){ k++; });
  return k;
}

vec<vec<bool>> PicoSolver::_GenerateModels() {
  vec<vec<bool>> models;
  ForAllModels(1, [&](){
    vec<bool> n(vars_.size(), false);
    for (uint id = 1; id < vars_.size(); id++) {
      if (picosat_deref(picosat_, id) == 1) n[id] = true;
    }
    models.push_back(n);
  });
  return models;
}

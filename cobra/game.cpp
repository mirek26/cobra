/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "parser.h"
#include "formula.h"
#include "game.h"

Experiment* g_e;
CnfFormula* g_init;

void Game::setVariables(VariableSet* vars) {
  variables_ = vars;
  for (auto var: *variables_) {
    var->orig(true);
  }
}

void print(std::vector<Variable*>& params) {
  for (auto v: params) {
    printf("%i ", v->id());
  }
  printf("\n");
  for (auto& outcome: g_e->outcomes()) {
    CnfFormula n;
    n.AddConstraint(*g_init);
    CnfFormula nnn = outcome.SubstituteParams(params);
    n.AddConstraint(nnn);
    if (!n.Satisfiable()) {
      printf("UNSAT.\n");
    } else {
      printf("SAT, FIXED %i\n", n.GetFixedVariables());
    }
  }
  printf("\n");
}

void Game::doAll() {

  CnfFormula f;
  f.AddConstraint(init_);
  g_init = &f;
  printf("CNF: %s\n", f.pretty().c_str());
  if (f.Satisfiable()) {
    printf("Satisfiable.\n");
    f.PrintAssignment();
  }

  std::map<int, int> equiv = f.ComputeVariableEquivalence();
  printf("Equivelnce classes:\n");
  for (auto& i: equiv) {
    printf("%i: %i\n", i.first, i.second);
  }

  for (auto& e: experiments_) {
  //auto& e = experiments_[0];
    g_e = e;
    printf("\n%s\n", e->name().c_str());
    e->param()->ForAll(print, equiv);
  }
}


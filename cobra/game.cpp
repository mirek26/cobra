/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "parser.h"
#include "formula.h"
#include "game.h"

void print(std::vector<Variable*>& params) {
  for (auto& v: params) {
    printf("%s ", v->pretty().c_str());
  }
  printf("\n");
}

void Game::doAll() {

  CnfFormula f;
  f.AddConstraint(init_);
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
    printf("\n%s\n", e->name().c_str());
    e->param()->ForAll(print, equiv);
  }
}


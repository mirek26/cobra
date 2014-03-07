/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "parser.h"
#include "formula.h"
#include "experiment.h"
#include "game.h"

// Experiment* g_e;
// CnfFormula* g_init;

void Game::declareVariable(Variable*) {

}

void Game::declareVariables(VariableList*) {

}

void Game::addRestriction(Formula*) {

}

Formula* Game::initialRestrictions() {
  return init_;
}

int Game::addMapping(std::string ident, VariableList* vars) {
  assert(mappings_ids_.count(ident) == 0);
  int new_id = mappings_.size();
  mappings_ids_[ident] = new_id;
  mappings_.push_back(std::vector<VarId>());
  for (auto v: *vars) {
    mappings_.back().push_back(v->id());
  }
  return new_id;
}

int Game::getMappingId(std::string ident) {
  //printf("Ask for %s.\n", ident.c_str());
  assert(mappings_ids_.count(ident) > 0);
  return mappings_ids_[ident];
}

int Game::getMappingValue(MapId mapping, CharId a) {
  assert(0 <= mapping && mapping < mappings_.size());
  assert(0 <= a && a < alphabet_.size());
  return mappings_[mapping][a];
}

Experiment* Game::addExperiment(std::string name, uint num_params) {
  auto e = new Experiment(this, name, num_params);
  experiments_.push_back(e);
  return e;
}


void Game::Precompute() {
  for (auto e: experiments_) {
    e->Precompute();
  }
}
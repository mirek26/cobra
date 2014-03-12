/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "parser.h"
#include "formula.h"
#include "experiment.h"
#include "game.h"

extern Parser m;

Game::Game() {
  restriction_ = m.get<AndOperator>();
}

void Game::declareVariable(Variable* var) {
  var->set_id(variables_.size());
  variables_.push_back(var);
  variables_ids_[var->ident()] = var->id();
}

void Game::declareVariables(std::vector<Variable*>* list) {
  for (auto var: *list) {
    var->set_id(variables_.size());
    variables_.push_back(var);
    variables_ids_[var->ident()] = var->id();
  }
  delete list;
}

Variable* Game::getVariableByName(std::string name) {
  m.input_assert(variables_ids_.count(name) > 0,
    "Undefined prepositional variable '" + name + "'.");
  return variables_[variables_ids_[name]];
}

void Game::addRestriction(Formula* f) {
  restriction_->addChild(f);
}

Formula* Game::restriction() {
  return restriction_;
}

int Game::addMapping(std::string ident, std::vector<Variable*>* vars) {
  m.input_assert(mappings_ids_.count(ident) == 0,
    "Mapping " + ident + " defined twice.");
  int new_id = mappings_.size();
  mappings_ids_[ident] = new_id;
  mappings_.push_back(std::vector<VarId>());
  for (auto v: *vars) {
    mappings_.back().push_back(getVariableByName(v->ident())->id());
  }
  return new_id;
}

int Game::getMappingId(std::string ident) {
  m.input_assert(mappings_ids_.count(ident) > 0,
    "Undefined mapping '" + ident + "'.");
  return mappings_ids_[ident];
}

int Game::getMappingValue(MapId mapping, CharId a) {
  assert(mapping < mappings_.size());
  assert(a >= 0 && a < alphabet_.size());
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
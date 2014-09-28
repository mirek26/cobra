/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <vector>
#include <string>
#include <map>
#include "./parser.h"
#include "./formula.h"
#include "./experiment.h"
#include "./game.h"

extern Parser m;


uint Game::bliss_calls = 0;
clock_t Game::bliss_time = 0;

Game::Game() {
  constraint_ = m.get<AndOperator>();
  vars_.push_back(nullptr);
}

void Game::declareVar(Variable* var) {
  var->set_id(vars_.size());
  vars_.push_back(var);
  vars_ids_[var->ident()] = var->id();
}

void Game::declareVars(vec<Variable*>* list) {
  for (auto var : *list)
    declareVar(var);
  delete list;
}

void Game::declareVars(std::initializer_list<string> list) {
  for (auto x : list)
    declareVar(m.get<Variable>(x));
}

Variable* Game::getVarByName(string name) const {
  m.input_assert(vars_ids_.count(name) > 0,
    "Undefined prepositional variable '" + name + "'.");
  return vars_[vars_ids_.at(name)];
}

void Game::addConstraint(Formula* f) {
  constraint_->addChild(f);
}

Formula* Game::constraint() const {
  return constraint_;
}

MapId Game::addMapping(string ident, vec<Variable*>* vars) {
  m.input_assert(mappings_ids_.count(ident) == 0,
    "Mapping " + ident + " defined twice.");
  int new_id = mappings_.size();
  mappings_ids_[ident] = new_id;
  mappings_.push_back(vec<VarId>());
  for (auto v : *vars) {
    mappings_.back().push_back(getVarByName(v->ident())->id());
  }
  return new_id;
}

MapId Game::getMappingId(string ident) const {
  m.input_assert(mappings_ids_.count(ident) > 0,
    "Undefined mapping '" + ident + "'.");
  return mappings_ids_.at(ident);
}

VarId Game::getMappingValue(MapId mapping, CharId a) const {
  assert(mapping < mappings_.size());
  assert(a < alphabet_.size());
  return mappings_[mapping][a];
}

ExpType* Game::addExperiment(string name, uint num_params) {
  auto e = new ExpType(*this, name, num_params);
  experiments_.push_back(e);
  return e;
}

string Game::ParamsToStr(const vec<CharId>& params, char sep) const {
  string s = "";
  if (params.empty()) return "NO PARAMS";
  for (auto a : params) {
    assert(a < alphabet_.size());
    s += alphabet_[a] + sep;
  }
  s.erase(s.length()-1, 1);
  return s;
}

void Game::Precompute() {
  for (auto e : experiments_) {
    e->Precompute();
  }
}

bliss::Graph* Game::CreateGraph() const {
  // Create the graph
  auto g = new bliss::Graph(0);
  // Add vertices for vars, create edge between a variable and its negation
  for (uint i = 1; i < vars().size(); i++) {
    g->add_vertex(vertex_type::kVariableId);
  }
  g->set_splitting_heuristic(bliss::Graph::shs_fsm);
  return g;
}

void Game::PrintModel(vec<bool> model) const {
  printf("TRUE: ");
  for (uint id = 1; id < vars_.size(); id++)
    if (model[id]) printf("%s ", vars_[id]->ident().c_str());
  printf("\nFALSE: ");
  for (uint id = 1; id < vars_.size(); id++)
    if (!model[id]) printf("%s ", vars_[id]->ident().c_str());
  printf("\n");
}

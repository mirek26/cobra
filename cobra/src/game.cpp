/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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
  vars_.push_back(nullptr);
}

void Game::declareVar(Variable* var) {
  var->set_id(vars_.size());
  vars_.push_back(var);
  vars_ids_[var->ident()] = var->id();
}

void Game::declareVars(vec<Variable*>* list) {
  for (auto var: *list)
    declareVar(var);
  delete list;
}

void Game::declareVars(std::initializer_list<string> list) {
  for (auto x: list)
    declareVar(m.get<Variable>(x));
}

Variable* Game::getVarByName(string name) {
  m.input_assert(vars_ids_.count(name) > 0,
    "Undefined prepositional variable '" + name + "'.");
  return vars_[vars_ids_[name]];
}

void Game::addRestriction(Formula* f) {
  restriction_->addChild(f);
}

Formula* Game::restriction() {
  return restriction_;
}

MapId Game::addMapping(string ident, vec<Variable*>* vars) {
  m.input_assert(mappings_ids_.count(ident) == 0,
    "Mapping " + ident + " defined twice.");
  int new_id = mappings_.size();
  mappings_ids_[ident] = new_id;
  mappings_.push_back(vec<VarId>());
  for (auto v: *vars) {
    mappings_.back().push_back(getVarByName(v->ident())->id());
  }
  return new_id;
}

MapId Game::getMappingId(string ident) {
  m.input_assert(mappings_ids_.count(ident) > 0,
    "Undefined mapping '" + ident + "'.");
  return mappings_ids_[ident];
}

VarId Game::getMappingValue(MapId mapping, CharId a) {
  assert(mapping < mappings_.size());
  assert(a < alphabet_.size());
  return mappings_[mapping][a];
}

ExpType* Game::addExperiment(string name, uint num_params) {
  auto e = new ExpType(*this, name, num_params);
  experiments_.push_back(e);
  return e;
}

string Game::ParamsToStr(const vec<CharId>& params, char sep) {
  string s = "";
  for (auto a: params)
    s += alphabet_[a] + sep;
  s.erase(s.length()-1, 1);
  return s;
}

void Game::Precompute() {
  for (auto e: experiments_) {
    e->Precompute();
  }
}

bliss::Graph* Game::CreateGraph() {
  // Create the graph
  auto g = new bliss::Graph(0);
  // Add vertices for vars, create edge between a variable and its negation
  for (uint i = 1; i < vars().size(); i++) {
    g->add_vertex(vertex_type::kVariableId);
  }
  g->set_splitting_heuristic(bliss::Graph::shs_fsm);
  return g;
}

void ComputeVarEquiv_NewGenerator(void* equiv, uint, const uint* aut) {
  vec<int>& var_equiv = *((vec<int>*) equiv);
  uint d = 0;
  for (uint i = 1; i < var_equiv.size(); i++) {
    if (aut[i - 1] <= i - 1) continue;
    if (d > 0) return;
    d = i;
  }
  if (d > 0 && aut[d - 1] + 1 < var_equiv.size()) {
    uint v1 = d, v2 = aut[d - 1] + 1;
    auto min = var_equiv[v2] > var_equiv[v1] ?
                  var_equiv[v1] : var_equiv[v2];
    var_equiv[v2] = var_equiv[v1] = min;
  }
}

vec<uint> Game::ComputeVarEquiv(bliss::Graph& graph) {
  bliss::Stats stats;
  vec<uint> var_equiv(vars_.size(), 0);
  for (uint i = 1; i < vars_.size(); i++) {
    var_equiv[i] = i;
  }
  clock_t t1 = clock();
  graph.find_automorphisms(stats,
                           ComputeVarEquiv_NewGenerator,
                           (void*)&var_equiv);
  bliss_calls_ += 1;
  bliss_time_ += clock() - t1;
  for (uint i = 1; i < vars_.size(); i++) {
    var_equiv[i] = var_equiv[var_equiv[i]];
  }
  return var_equiv;
}

void Game::PrintModel(vec<bool> model) {
  printf("TRUE: ");
  for (uint id = 1; id < vars_.size(); id++)
    if (model[id]) printf("%s ", vars_[id]->ident().c_str());
  printf("\nFALSE: ");
  for (uint id = 1; id < vars_.size(); id++)
    if (!model[id]) printf("%s ", vars_[id]->ident().c_str());
  printf("\n");
}
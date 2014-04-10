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
  var->set_id(variables_.size() + 1);
  variables_.push_back(var);
  variables_ids_[var->ident()] = var->id();
}

void Game::declareVariables(vec<Variable*>* list) {
  for (auto var: *list)
    declareVariable(var);
  delete list;
}

void Game::declareVariables(std::initializer_list<string> list) {
  for (auto x: list)
    declareVariable(m.get<Variable>(x));
}

Variable* Game::getVariableByName(string name) {
  m.input_assert(variables_ids_.count(name) > 0,
    "Undefined prepositional variable '" + name + "'.");
  return variables_[variables_ids_[name] - 1];
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
    mappings_.back().push_back(getVariableByName(v->ident())->id());
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

Experiment* Game::addExperiment(string name, uint num_params) {
  auto e = new Experiment(*this, name, num_params);
  experiments_.push_back(e);
  return e;
}


void Game::Precompute() {
  for (auto e: experiments_) {
    e->Precompute();
  }
}

bliss::Digraph* Game::CreateGraph() {
  int new_id = 0;
  // Create the graph
  auto g = new bliss::Digraph(0);
  // Add vertices for variables, create edge between a variable and its negation
  for (auto var: variables()) {
    g->add_vertex(var->node_id());
    g->add_vertex(var->node_id());
    g->add_edge(new_id, new_id + 1);
    g->add_edge(new_id + 1, new_id);
    new_id += 2;
  }
  g->set_splitting_heuristic(bliss::Digraph::shs_fsm);
  return g;
}

void ComputeVarEquiv_NewGenerator(void* equiv, uint, const uint* aut) {
  vec<int>& var_equiv = *((vec<int>*) equiv);
  int d = -1;
  for (uint i = 0; i < var_equiv.size()*2 - 2; i+=2) {
    if (aut[i] <= i) continue;
    if (aut[i+1] != aut[i]+1) return;
    if (d > -1) return;
    d = i;
  }
  uint v1 = aut[d]/2 + 1, v2 = d/2 + 1;
  if (v1 < var_equiv.size()) {
    auto min = var_equiv[v1] > var_equiv[v2] ? var_equiv[v2] : var_equiv[v1];
    var_equiv[v1] = var_equiv[v2] = min;
  }
}

vec<uint> Game::ComputeVarEquiv(bliss::Digraph& graph) {
  bliss::Stats stats;
  auto var_count = variables_.size();
  vec<uint> var_equiv(var_count + 1, 0);
  for (uint i = 1; i <= var_count; i++) {
    var_equiv[i] = i;
  }
  graph.find_automorphisms(stats,
                           ComputeVarEquiv_NewGenerator,
                           (void*)&var_equiv);
  //printf("VAREQUIV:\n");
  for (uint i = 1; i <= var_count; i++) {
    var_equiv[i] = var_equiv[var_equiv[i]];
    //printf("%i : %i \n", i, var_equiv[i]);
  }
  //printf("\n");
  return var_equiv;
}

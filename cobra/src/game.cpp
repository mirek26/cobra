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

Experiment* Game::addExperiment(string name, uint num_params) {
  auto e = new Experiment(*this, name, num_params);
  experiments_.push_back(e);
  return e;
}

void Game::PrintCode(vec<bool> code) {
  vec<int> trueVar;
  vec<int> falseVar;
  assert(code.size() == vars_.size());
  for (uint id = 1; id < vars_.size(); id++) {
    if (code[id] == 1) trueVar.push_back(id);
    else falseVar.push_back(id);
  }
  printf("TRUE: ");
  for (auto s: trueVar) printf("%s ", vars_[s]->ident().c_str());
  printf("\nFALSE: ");
  for (auto s: falseVar) printf("%s ", vars_[s]->ident().c_str());
  printf("\n");
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
  // Add vertices for vars, create edge between a variable and its negation
  for (auto it = vars().begin() + 1; it != vars().end(); ++it) {
    g->add_vertex((*it)->type_id());
    g->add_vertex((*it)->type_id());
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

vec<uint> Game::ComputeVarEquiv(Solver& solver, bliss::Digraph& graph) {
  bliss::Stats stats;
  auto var_count = vars_.size();
  vec<uint> var_equiv(var_count, 0);
  // Assign label t/f to fixed vars, label others by their position
  auto t = var_count, f = var_count;
  for (uint i = 1; i < var_count; i++) {
    if (solver.MustBeTrue(i)) var_equiv[i] = t;
    else if (solver.MustBeFalse(i)) var_equiv[i] = f;
    var_equiv[i] = i;
  }
  clock_t t1 = clock();
  graph.find_automorphisms(stats,
                           ComputeVarEquiv_NewGenerator,
                           (void*)&var_equiv);
  bliss_calls_ += 1;
  bliss_time_ += clock() - t1;
  for (uint i = 1; i < var_count; i++) {
    var_equiv[i] = var_equiv[var_equiv[i]];
  }

  return var_equiv;
}

vec<Option> Game::GenerateExperiments(Solver& solver, bliss::Digraph& graph) {
  auto var_equiv = ComputeVarEquiv(solver, graph);
  // printf("Var equiv:\n");
  // for (auto i: var_equiv) printf("%i ", i);
  // printf("\n");

  // Prepare list of all sensible experiments in this round.
  vec<Option> options;
  for (auto e: experiments_) {
    auto& params_all = e->GenParams(var_equiv);
    for (auto& params: params_all) {
      options.push_back(Option(solver, *e, params, options.size()));
    }
  }
  assert(options.size() > 0);
  return options;
}

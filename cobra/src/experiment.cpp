/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <vector>
#include <unordered_map>
#include <map>
#include <exception>
#include <cassert>
#include <string>
#include <set>
#include <bliss/graph.hh>
#include <bliss/utils.hh>
#include <permlib/permutation.h>
#include "./common.h"
#include "./formula.h"
#include "./game.h"
#include "./parser.h"
#include "./experiment.h"
#include <iostream>

extern Parser m;

Experiment::Experiment(Solver& solver, const ExpType& e,
                       vec<CharId> params, uint index):
  solver_(&solver),
  type_(&e),
  params_(params),
  index_(index) {
  data_.resize(e.outcomes().size());
}

bool Experiment::IsSat(uint id) {
  assert(id < data_.size());
  if (!data_[id].sat_c) {
    solver_->OpenContext();
    solver_->AddConstraint(type_->outcomes()[id].formula, params_);
    data_[id].sat = solver_->Satisfiable();
    data_[id].sat_c = true;
    solver_->CloseContext();
  }
  return data_[id].sat;
}

bool Experiment::IsFinalSat() {
  if (type_->final_outcome() == -1)
    return true;
  else
    return IsSat(type_->final_outcome());
}

uint Experiment::NumOfSat() {
  uint total = 0;
  for (uint i = 0; i < data_.size(); i++)
    total += IsSat(i);
  return total;
}

uint Experiment::NumOfModels(uint id) {
  assert(id < data_.size());
  if (!data_[id].models_c) {
    solver_->OpenContext();
    solver_->AddConstraint(type_->outcomes()[id].formula, params_);
    data_[id].models = solver_->NumOfModels();
    data_[id].models_c = true;
    data_[id].sat = data_[id].models > 0;
    data_[id].sat_c = true;
    solver_->CloseContext();
  }
  return data_[id].models;
}

uint Experiment::TotalNumOfModels() {
  uint total = 0;
  for (uint i = 0; i < data_.size(); i++)
    total += NumOfModels(i);
  return total;
}

uint Experiment::MaxNumOfModels() {
  uint max = 0;
  for (uint i = 0; i < data_.size(); i++)
    max = std::max(max, NumOfModels(i));
  return max;
}

uint Experiment::NumOfFixedVars(uint id) {
  assert(id < data_.size());
  if (!data_[id].fixed_c) {
    solver_->OpenContext();
    solver_->AddConstraint(type_->outcomes()[id].formula, params_);
    data_[id].fixed = solver_->GetNumOfFixedVars();
    data_[id].fixed_c = true;
    solver_->CloseContext();
  }
  return data_[id].fixed;
}

string Experiment::pretty() {
    return type().name() + " " + type().game().ParamsToStr(params_);
}

ExpType::ExpType(const Game& game, string name, uint num_params)
  : game_(game),
    name_(name),
    num_params_(num_params),
    final_outcome_(-1),
    positions_dep_(num_params) {
  params_different_.resize(num_params);
  params_smaller_than_.resize(num_params);
  alph_ = game_.alphabet().size();
}

void ExpType::addOutcome(string name, Formula* outcome, bool final) {
  if (final) final_outcome_ = outcomes_.size();
  outcomes_.push_back({ name, outcome, final });
  uint final_count = 0;
  for (auto o : outcomes_) final_count += o.final;
  if (final_count > 1) final_outcome_ = -1;
}

void ExpType::paramsDistinct(vec<uint>* list) {
  for (auto i = list->begin(); i != list->end(); ++i) {
    for (auto j = i + 1; j != list->end(); ++j) {
      m.input_assert(*i > 0 && *i <= num_params_,
        "Invalid parameter id in PARAMS_DISTINCT.");
      m.input_assert(*j > 0 && *j <= num_params_,
        "Invalid parameter id in PARAMS_DISTINCT.");
      // params are internally indexed from 0, that's why *i - 1
      params_different_[*i - 1].insert(*j - 1);
      params_different_[*j - 1].insert(*i - 1);
    }
  }
  delete list;
}

void ExpType::paramsSorted(vec<uint>* list) {
  for (auto i = list->begin(); i != list->end(); ++i) {
    for (auto j = i + 1; j != list->end(); ++j) {
      m.input_assert(*i > 0 && *i < *j && *j <= num_params_,
        "Invalid parameter id or invalid order in PARAMS_SORTED.");
      // params are internally indexed from 0, that's why *i - 1
      params_smaller_than_[*j - 1].insert(*i - 1);
    }
  }
  delete list;
}

uint64_t ExpType::NumberOfParametrizations() const {
  uint64_t total = 1;
  for (uint i = 0; i < num_params_; i++) {
    int pos = alph_;
    for (auto p : params_different_[i]) {
      if (p < i) pos--;
    }
    total *= pos;
  }
  return total;
}

void ExpType::PrecomputeUsed(Formula* f) {
  assert(f);
  auto* mapping = dynamic_cast<Mapping*>(f);
  auto* variable = dynamic_cast<Variable*>(f);
  if (mapping) {
    // printf("%i %i\n", mapping->param_id(), num_params_);
    assert(mapping->param_id() < num_params_);
    maps_at_positions_[mapping->param_id()][mapping->mapping_id()] = true;
  } else if (variable) {
    used_vars_.insert(variable->id());
  } else {
    for (auto c : f->children())
      PrecomputeUsed(c);
  }
}

void ExpType::Precompute() {
  // used_maps_, used_vars_
  maps_at_positions_.insert(maps_at_positions_.begin(), num_params_, 
                           vec<bool>(game_.numMappings(), false));
  for (auto out : outcomes_) {
    PrecomputeUsed(out.formula);
  }

  for (MapId m = 0; m < game_.numMappings(); m++) {
    for (uint i = 0; i < num_params_; i++) {
      for (uint j = 0; j < num_params_; j++) {
        if (maps_at_positions_[i][m] && maps_at_positions_[j][m]) 
          positions_dep_.merge(i, j);
      } 
    }
  }

  // for (uint i = 0; i < num_params_; i++) {
  //   printf("POS %i: %i\n", i, positions_dep_.root(i));
  // }
}

void NewSymGenerator(void* gen, uint n, const uint* aut) {
  vec<vec<uint>>& generators = *((vec<vec<uint>>*) gen);
  generators.push_back(vec<uint>(aut, aut + n));
}

ExpGenerator::ExpGenerator(const Game& game, Solver& solver,
                           const vec<EvalExp>& history, bool use_bliss)
  : game_(game),
    solver_(solver),
    use_bliss_(use_bliss) {
  stats_ = GenParamsStats();
  
  // Construct the base graph
  graph_ = game.CreateGraph();
  fixed_vars_ = solver.GetFixedVars();
  for (auto id : fixed_vars_) {
    graph_->change_color(abs(id) - 1,
              id < 0 ? vertex_type::kFalseVar : vertex_type::kTrueVar);
  }
  game.constraint()->PropagateFixed(fixed_vars_, nullptr);
  game.constraint()->AddToGraphRooted(*graph_, nullptr,
                                      vertex_type::kKnowledgeRoot);
  for (auto& e : history) {
    auto formula = e.exp.type().outcomes()[e.outcome_id].formula;
    formula->PropagateFixed(fixed_vars_, &e.exp.params());
    formula->AddToGraphRooted(*graph_, &e.exp.params(),
                              vertex_type::kKnowledgeRoot);
  }

  for (auto e : game.experiments()) {
    for (uint p = 0; p < e->num_params(); p++) {
      for (uint u = 0; u < game.numMappings(); u++) {
        for (uint v = 0; v < game.numMappings(); v++) {
          if (u == v || !e->map_at(p, u) || !e->map_at(p, v)) continue;
          for (uint i = 0; i < game.alphabet().size(); i++) {
            auto v1 = game.getMappingValue(u, i);
            auto v2 = game.getMappingValue(v, i);
            graph_->add_edge(v1 - 1, v2 - 1);
          }
        }
      }
    }
  }

  // Compute symmetry group
  vec<vec<uint>> generators;
  bliss::Stats stats;
  graph_->find_automorphisms(stats,
                             NewSymGenerator,
                             reinterpret_cast<void*>(&generators));

  std::list<Perm::ptr> group_generators;
  for (auto& g: generators) {
    Perm::ptr gen(new Perm(g.begin(), g.begin() + game_.vars().size()-1));
    group_generators.push_back(gen);
  }

  // BSGS construction
  permlib::SchreierSimsConstruction<Perm, Transversal> schreier_sims(game_.vars().size()-1);
  symmetry_bsgs_ = new permlib::BSGS<Perm, Transversal>(schreier_sims.construct(
                         group_generators.begin(), group_generators.end()));
  
}

ExpGenerator::~ExpGenerator() {
  delete graph_;
  for (auto& g : graphs_) {
    delete g.first;
  }
  if (symmetry_bsgs_) delete symmetry_bsgs_;
}

vec<Experiment> ExpGenerator::All() {
  for (auto t : game_.experiments()) {
    curr_type_ = t;

    params_.resize(t->num_params());
    GenParamsFill(0);
  }
  // printf("PH1: %i\n", stats_.ph1);
  return experiments_;
}

bool ExpGenerator::TestDominance(int n, int a, int b) {
  for (int i = 0; i < n; i++) {
    if (curr_type_->pos_dep(n, i) && (params_[i] == a || params_[i] == b))
      return false;
  }
  Perm perm(game_.vars().size() - 1); // vars in perm are 0-based
  for (uint f = 0; f < game_.numMappings(); f++) {
    for (uint p = 0; p < curr_type_->num_params(); p++) {
      if (!curr_type_->pos_dep(n, p)) continue;
      if (curr_type_->map_at(p, f)) {
        perm.setTransposition(game_.getMappingValue(f, a) - 1,
                              game_.getMappingValue(f, b) - 1);
        break;
      }
    }
  }

  // std::cout << "Asking for permutation " << perm << std::endl;
  assert(symmetry_bsgs_);
  string s = game_.ParamsToStr(params_), t = game_.ParamsToStr(params_);
  s.erase(2*n); s += game_.alphabet()[a];
  t.erase(2*n); t += game_.alphabet()[b]; 
  if (symmetry_bsgs_->sifts(perm)) {
    // printf("Prefix %s dominated by %s. Perm ", s.c_str(), t.c_str());
    // std::cout << perm << std::endl;
    return true;
  } else {
    // printf("!!! Prefix %s NOT dominated by %s. Perm ", s.c_str(), t.c_str());
    // std::cout << perm << std::endl;
    return false;
  }
}

// Recursive function that substitudes char at position n for all posibilities.
void ExpGenerator::GenParamsFill(uint n) {
  set<CharId> done;
  if (n == curr_type_->num_params()) {
    stats_.ph1++;
    // run symmetri detection ONLY if use_bliss_ set to true
    if (use_bliss_) {
      GenParamsGraphFilter();
    } else {
      experiments_.push_back({ solver_, *curr_type_, params_,
                               static_cast<uint>(experiments_.size()) });
    }
    return;
  }
  for (CharId a = 0; a < game_.alphabet().size(); a++) {
    bool valid = true;
    // Test compliance with PARAMS_DIFFERENT.
    for (auto p : curr_type_->params_different_[n]) {
      if (p < n && params_[p] == a) valid = false;
    }
    if (!valid) continue;
    // Test compliance with PARAMS_SORTED.
    for (auto p : curr_type_->params_smaller_than_[n]) {
      assert(p < n);
      if (params_[p] > a) valid = false;
    }
    if (!valid) continue;
    // Phase 1 equivalence testing.
    for (auto b: done) {
      if (TestDominance(n, a, b)) {
        valid = false;
        break;
      }
    }
    if (!valid) continue;
    done.insert(a);
    // Recurse down.
    params_[n] = a;
    GenParamsFill(n + 1);
  }
}

void ExpGenerator::GenParamsGraphFilter() {
  bliss::Stats stats;
  auto graph = bliss::Graph(*graph_);
  for (auto outcome : curr_type_->outcomes()) {
    outcome.formula->PropagateFixed(fixed_vars_, &params_);
    outcome.formula->AddToGraphRooted(graph, &params_,
                                      vertex_type::kOutcomeRoot);
  }

  clock_t t1 = clock();
  auto canonical = graph.permute(graph.canonical_form(stats, nullptr, nullptr));
  Game::bliss_calls += 1;
  Game::bliss_time += clock() - t1;

  if (graphs_.count(canonical) > 0 &&
      experiments_[graphs_[canonical]].IsFinalSat()) {
    delete canonical;
    return;
  }

  Experiment e(solver_, *curr_type_, params_, experiments_.size());
  if (graphs_.count(canonical) == 0) {
    graphs_[canonical] = experiments_.size();
    experiments_.push_back(e);
    stats_.ph3++;
  } else if (e.IsFinalSat()) {
    experiments_[graphs_[canonical]] = e;
  } else {
    delete canonical;
  }
}

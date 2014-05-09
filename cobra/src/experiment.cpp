/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <vector>
#include <map>
#include <exception>
#include <cassert>
#include <bliss/graph.hh>
#include <bliss/utils.hh>

#include "common.h"
#include "formula.h"
#include "game.h"
#include "parser.h"
#include "experiment.h"

extern Parser m;

Experiment::Experiment(Solver& solver, ExpType& e, vec<CharId> params, uint index):
  solver_(solver),
  type_(e),
  params_(params),
  index_(index) {
  data_.resize(e.outcomes().size());
}

bool Experiment::IsSat(uint id) {
  assert(id < data_.size());
  if (!data_[id].sat_c) {
    solver_.OpenContext();
    solver_.AddConstraint(type_.outcomes()[id].formula, params_);
    data_[id].sat = solver_.Satisfiable();
    data_[id].sat_c = true;
    solver_.CloseContext();
  }
  return data_[id].sat;
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
    solver_.OpenContext();
    solver_.AddConstraint(type_.outcomes()[id].formula, params_);
    data_[id].models = solver_.NumOfModels();
    data_[id].models_c = true;
    data_[id].sat = data_[id].models > 0;
    data_[id].sat_c = true;
    solver_.CloseContext();
  }
  return data_[id].models;
}

uint Experiment::TotalNumOfModels() {
  uint total = 0;
  for (uint i = 0; i < data_.size(); i++)
    total += NumOfModels(i);
  return total;
}

uint Experiment::NumOfFixedVars(uint id) {
  assert(id < data_.size());
  if (!data_[id].fixed_c) {
    solver_.OpenContext();
    solver_.AddConstraint(type_.outcomes()[id].formula, params_);
    data_[id].fixed = solver_.GetNumOfFixedVars();
    data_[id].fixed_c = true;
    solver_.CloseContext();
  }
  return data_[id].models;
}


void ExpType::addOutcome(string name, Formula* outcome, bool final) {
  if (final) final_outcome_ = outcomes_.size();
  outcomes_.push_back({ name, outcome, final });
}

void ExpType::paramsDistinct(vec<uint>* list) {
  for(auto i = list->begin(); i != list->end(); ++i) {
    for(auto j = i + 1; j != list->end(); ++j) {
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
  for(auto i = list->begin(); i != list->end(); ++i) {
    for(auto j = i + 1; j != list->end(); ++j) {
      m.input_assert(*i > 0 && *i < *j && *j <= num_params_,
        "Invalid parameter id or invalid order in PARAMS_SORTED.");
      // params are internally indexed from 0, that's why *i - 1
      params_smaller_than_[*j - 1].insert(*i - 1);
    }
  }
  delete list;
}

void ExpType::PrecomputeUsed(Formula* f) {
  assert(f);
  auto* mapping = dynamic_cast<Mapping*>(f);
  auto* variable = dynamic_cast<Variable*>(f);
  if (mapping) {
    //printf("%i %i\n", mapping->param_id(), num_params_);
    assert(mapping->param_id() < num_params_);
    used_maps_[mapping->param_id()].insert(mapping->mapping_id());
  } else if (variable) {
    used_vars_.insert(variable->id());
  } else {
    for (uint i = 0; i < f->child_count(); i++)
      PrecomputeUsed(f->child(i));
  }
}

void ExpType::Precompute() {
  // used_maps_, used_vars_
  used_maps_.resize(num_params_);
  for (auto out: outcomes_) {
    PrecomputeUsed(out.formula);
  }

  // interchangable
  interchangable_.resize(num_params_);
  for (uint d = 0; d < num_params_; d++) {
    interchangable_[d].resize(alph_);
    for (CharId a = 0; a < alph_; a++) {
      interchangable_[d][a] = true;
      set<int> vars;
      for (auto& f: used_maps_[d]) {
        auto var = game_.getMappingValue(f, a);
        vars.insert(var);
        if (used_vars_.count(var) > 0) interchangable_[d][a] = false;
      }
      for (uint e = 0; e < num_params_; e++) {
        if (e == d) continue;
        for (CharId b = 0; b < alph_; b++) {
          // check that _b_ can be at _e_, given that _a_ is at _d_
          if (a == b && params_different_[e].count(d) > 0) continue;
          if (a >= b && params_smaller_than_[e].count(d) > 0) continue;
          for (auto f: used_maps_[e]) {
            if (vars.count(game_.getMappingValue(f, b)) > 0)
              interchangable_[d][a] = false;
          }
        }
      }
      //printf("%s %i %i -> %s\n", name_.c_str(), d, a, interchangable_[d][a] ? "true" : "false");
    }
  }
}

bool ExpGenerator::CharsEquiv(set<MapId>& maps, CharId a, CharId b) const {
  bool equiv = true;
  for (auto f: maps) {
    if (var_groups_[game_.getMappingValue(f, a)] !=
        var_groups_[game_.getMappingValue(f, b)]) {
      equiv = false;
    }
  }
  return equiv;
}

// Recursive function that substitudes char at position n for all posibilities.
void ExpGenerator::GenParamsFill(uint n) {
  set<CharId> done;
  for (CharId a = 0; a < game_.alphabet().size(); a++) {
    bool valid = true;
    // Test compliance with PARAMS_DIFFERENT.
    for (auto p: curr_type_->params_different_[n]) {
      if (p < n && params_[p] == a) valid = false;
    }
    if (!valid) continue;
    done.insert(a);
    // Test compliance with PARAMS_SORTED.
    for (auto p: curr_type_->params_smaller_than_[n]) {
      assert(p < n);
      if (params_[p] > a) valid = false;
    }
    if (!valid) continue;
    // Test equivalence.
    if (curr_type_->interchangable_[n][a]) {
      for (auto b: done) {
        if (a == b) continue;
        if (CharsEquiv(curr_type_->used_maps_[n], a, b)) {
          valid = false;
          done.erase(a);  // there is an equivalent in done -> not needed
          break;
        }
      }
    }
    if (!valid) continue;
    // Recurse down.
    params_[n] = a;
    if (n + 1 == curr_type_->num_params()) {
      stats_.ph1++;
      //GenParamsBasicFilter();
      GenParamsGraphFilter();
    } else {
      GenParamsFill(n + 1);
    }
  }
}

void ExpGenerator::GenParamsBasicFilter() {
  // Compute position partitioning according to the vars.
  auto num_params = curr_type_->num_params();
  vec<uint> dfu(num_params, 0);
  for (uint i = 0; i < num_params; i++) dfu[i] = i;
  for (uint i = 0; i < num_params; i++)
    for (uint j = i + 1; j < num_params; j++)
      for (auto fi: curr_type_->used_maps_[i])
        for (auto fj: curr_type_->used_maps_[j])
          if (game_.getMappingValue(fi, params_[i]) ==
              game_.getMappingValue(fj, params_[j])) {
            if (dfu[i] > dfu[j])
              dfu[i] = dfu[j];
            else
              dfu[j] = dfu[i];
          }
  // Compute canonical form of the parametrization.
  for (uint n = 0; n < num_params; n++) {
    if (dfu[n] != n) continue; // only for roots of components
    CharId chr = params_[n];
    if (curr_type_->interchangable_[n][chr]) continue;
    // Precompute minimal set of positions such that...
    bool keep = false;
    set<MapId> maps;
    set<VarId> other_vars;
    for (uint p = 0; p < num_params; p++) {
      if (dfu[p] == n) {
        if (params_[p] != chr) keep = true;
        for (auto f: curr_type_->used_maps_[p]) {
          if (curr_type_->used_vars_.count(game_.getMappingValue(f, chr))) keep = true;
          maps.insert(f);
        }
      } else {
        for (auto f: curr_type_->used_maps_[p]) {
          other_vars.insert(game_.getMappingValue(f, params_[p]));
        }
      }
    }
    if (keep) continue;
    keep = true;
    // Try substituing the whole group with other char (from 0).
    for (CharId a = 0; a < chr; a++) {
      // Is it equivalent?
      if (!CharsEquiv(maps, a, chr)) continue;
      // Doesn't it indroduce conflicting vars?
      keep = false;
      for (auto f: maps) {
        VarId var = game_.getMappingValue(f, a);
        if (other_vars.count(var)) keep = true;
      }
      if (!keep) {
        // Substitude the whole group.
        for (uint p = 0; p < num_params; p++)
          if (dfu[p] == n) params_[p] = a;
        break;
      }
    }
  }
  // If the canonical form is not yet present, add it.
  if (params_basic_.count(params_) == 0) {
    params_basic_.insert(params_);
    stats_.ph2++;
    GenParamsGraphFilter();
  }
}

void ExpGenerator::GenParamsGraphFilter() {
  bliss::Stats stats;
  auto graph = bliss::Graph(*graph_);
  for (auto outcome: curr_type_->outcomes()) {
    outcome.formula->PropagateFixed(fixed_vars_, &params_);
    outcome.formula->AddToGraph(graph, &params_, vertex_type::kOutcomeRoot);
  }

  clock_t t1 = clock();
  auto canonical = graph.permute(graph.canonical_form(stats, nullptr, nullptr));
  game_.bliss_calls() += 1;
  game_.bliss_time() += clock() - t1;

  auto h = canonical->get_hash();
  if (graphs_.count(h) > 0 && graphs_[h].second) {
    delete canonical;
    return;
  }

  auto final = curr_type_->final_outcome();
  auto finalsat = true;
  if (final > -1) {
    // TODO: optimize...
    solver_.OpenContext();
    solver_.AddConstraint(curr_type_->outcomes()[final].formula, params_);
    finalsat = solver_.Satisfiable();
    solver_.CloseContext();
  }

  if (graphs_.count(h) == 0 || finalsat) {
    // TODO: proper hash map with graph comparison
    //graph.write_dot((game_.ParamsToStr(params_, '_')+".dot").c_str());
    graphs_[h] = make_pair(params_, finalsat);
    stats_.ph3++;
    params_final_.insert(params_);
  }

  delete canonical;
}

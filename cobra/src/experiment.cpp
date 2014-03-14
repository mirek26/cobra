/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
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

void Experiment::addOutcome(std::string name, Formula* outcome) {
  outcomes_names_.push_back(name);
  outcomes_.push_back(outcome);
}

void Experiment::paramsDistinct(std::vector<uint>* list) {
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

void Experiment::paramsSorted(std::vector<uint>* list) {
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

void Experiment::PrecomputeUsed(Formula* f) {
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

void Experiment::Precompute() {
  // used_maps_, used_vars_
  used_maps_.resize(num_params_);
  for (auto out: outcomes_) {
    PrecomputeUsed(out);
  }

  // interchangable
  interchangable_.resize(num_params_);
  for (uint d = 0; d < num_params_; d++) {
    interchangable_[d].resize(alph_);
    for (CharId a = 0; a < alph_; a++) {
      interchangable_[d][a] = true;
      std::set<int> vars;
      for (auto& f: used_maps_[d]) {
        auto var = game_->getMappingValue(f, a);
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
            if (vars.count(game_->getMappingValue(f, b)) > 0)
              interchangable_[d][a] = false;
          }
        }
      }
      //printf("%s %i %i -> %s\n", name_.c_str(), d, a, interchangable_[d][a] ? "true" : "false");
    }
  }
}

bool Experiment::CharsEquivalent(uint n, CharId a, CharId b, std::vector<int>& groups) const {
  bool equiv = true;
  for (auto f: used_maps_[n]) {
    if (groups[game_->getMappingValue(f, a)] !=
        groups[game_->getMappingValue(f, b)]) {
      equiv = false;
    }
  }
  return equiv;
}

// Recursive function that substitudes char at position n for all posibilities.
void Experiment::FillParametrization(std::vector<int>& groups, uint n) {
  std::set<CharId> done;
  for (CharId a = 0; a < alph_; a++) {
    bool recurse = true;
    // Test whether this agrees with PARAMS_DIFFERENT.
    for (auto p: params_different_[n]) {
      if (p < n && tmp_params_[p] == a) recurse = false;
    }
    if (!recurse) {
      // for (int i = 0; i < n; i++) printf("%i ", tmp_params_[i]); printf("%i not different. \n", a);
      continue;
    }
    done.insert(a);
    // Test whether this agrees with PARAMS_SORTED.
    for (auto p: params_smaller_than_[n]) {
      assert(p < n);
      if (tmp_params_[p] > a) recurse = false;
    }
    if (!recurse) {
      // for (int i = 0; i < n; i++) printf("%i ", tmp_params_[i]); printf("%i not bigger. \n", a);
      continue;
    }
    // Test equivalence.
    if (interchangable_[n][a]) {
      for (auto b: done) {
        if (a == b) continue;
        if (CharsEquivalent(n, a, b, groups)) {
          recurse = false;
          // for (int i = 0; i < n; i++) printf("%i ", tmp_params_[i]);
          // printf("%i ~ %i.\n", a, b);
          done.erase(a);  // there is an equivalent in done -> not needed
          break;
        }
      }
    }
    if (!recurse) continue;
    // Recurse down.
    tmp_params_[n] = a;
    if (n + 1 == num_params_) {
      tmp_params_all_.insert(tmp_params_);
      //for (auto p: tmp_params_) printf("%i ", p); printf(".\n");
    } else {
      // for (int i = 0; i <= n; i++) printf("%i ", tmp_params_[i]);
      // printf("...\n");
      FillParametrization(groups, n + 1);
    }
  }
}

std::set<std::vector<CharId>>*
Experiment::GenerateParametrizations(std::vector<int> groups) {
  tmp_params_.resize(num_params_);
  tmp_params_all_.clear();
  std::set<std::vector<CharId>> remove_params;
  FillParametrization(groups, 0);
  for (uint n = 0; n < num_params_; n++) {
    for (auto params: tmp_params_all_) {
      //for (auto p: params) printf("%i ", p); printf("change pos %i - ", n);

      CharId chr = params[n];
      if (interchangable_[n][chr]) continue;
      // Build other_vars - vars in outcome formulas due to other positions.
      std::set<VarId> other_vars(used_vars_);
      for (uint i = 0; i < num_params_; i++) {
        if (i == n) continue;
        for (auto f: used_maps_[i]) {
          other_vars.insert(game_->getMappingValue(f, params[i]));
        }
      }
      //
      bool keep = false;
      for (auto f: used_maps_[n]) {
        VarId var = game_->getMappingValue(f, chr);
        if (other_vars.count(var) > 0) keep = true;
      }
      if (keep) continue;
      // Consider alternatives.
      for (CharId a = 0; a < chr; a++) {
        params[n] = a;
        if (tmp_params_all_.count(params) == 0) continue;
        if (!CharsEquivalent(n, a, chr, groups)) continue;
        keep = false;
        for (auto f: used_maps_[n]) {
          VarId var = game_->getMappingValue(f, a);
          if (other_vars.count(var) > 0) keep = true;
        }
        if (!keep) {
          // Schedule for removal.
          params[n] = chr;
          remove_params.insert(params);
          break;
        }
      }
    }
    for (auto& params: remove_params) {
      tmp_params_all_.erase(params);
    }
    remove_params.clear();
  }
  // printf("===\n");
  // for (auto& params: tmp_params_all_) {
  //   for (auto p: params) printf("%i ", p);
  //   printf("\n");
  // }

  // ---
  std::map<unsigned int, std::vector<CharId>> hash;
  for (auto params: tmp_params_all_) {
  //decltype(tmp_params_all_) test = { {0,0,1} };
  //for (auto params: test) {
    bliss::Stats stats;
    auto g = BlissGraphForParametrization(groups, params);
    // g->write_dimacs(stdout);
    auto a = g->canonical_form(stats, nullptr, nullptr);
    auto ng = g->permute(a);
    // ng->write_dimacs(stdout);
    //printf("\n\n");
    auto h = ng->get_hash();
    //for (auto p: params) printf("%i ", p);
    if (hash.count(h) > 0) {
      remove_params.insert(params);
      //printf("..seems to be equiv to.. ");
      //for (auto p: hash[h]) printf("%i ", p);
      //bliss::print_permutation(stdout, g->get_nof_vertices(), a, 1);
      //printf("\n");
    } else {
      //printf("is new!\n");
      hash[h] = params;
    }
  }
  for (auto& params: remove_params) {
    tmp_params_all_.erase(params);
  }
  remove_params.clear();

  // for (auto p: hash) {
  //   for (auto x: p.second) printf("%i ", x);
  //   printf(".\n");
  // }
  return &tmp_params_all_;
}

bliss::Digraph* Experiment::BlissGraphForParametrization(
                          std::vector<int>& groups,
                          std::vector<CharId>& params) {
  std::map<Formula*, uint> node_ids;
  std::map<uint, uint> var_ids;
  int new_id = 0;
  // Create the graph
  auto g = new bliss::Digraph(0);
  // Add vertices for variables, create edge between a variable and its negation
  int max_group_id = 0;
  for (auto var: game_->variables()) {
    // printf("Adding vertices for variable %s - color %i\n", var->pretty().c_str(), groups[var->id()]);
    if (groups[var->id()] > max_group_id) max_group_id = groups[var->id()];
    g->add_vertex(groups[var->id()]);
    g->add_vertex(groups[var->id()]);
    var_ids[var->id()] = new_id;
    // printf("Adding edge %i - %i\n", new_id, new_id+1 );
    g->add_edge(new_id, new_id + 1);
    g->add_edge(new_id + 1, new_id);
    new_id += 2;
  }
  // Go through all outcome formulas and create nodes.
  std::function<void(Formula*)> CreateNodes = [&](Formula* c) {
    if (c->isLiteral()) return;
    if (node_ids.count(c) == 0) {
      node_ids[c] = new_id++;
      // printf("add VERTEX %i for %s\n", new_id-1, c->pretty().c_str());
      g->add_vertex(max_group_id + c->node_id());
    }
    for (uint i = 0; i < c->child_count(); i++) CreateNodes(c->child(i));
  };
  for (auto outcome: outcomes_) {
    CreateNodes(outcome);
  }
  // Create all other edges according to the structure of the formula
  std::function<void(Formula*)> AddEdges = [&](Formula* c) {
    for (uint i = 0; i < c->child_count(); i++) {
      auto ch = c->child(i);
      bool neg = false;
      if (ch->isLiteral()) {
        if (dynamic_cast<NotOperator*>(ch)) {
          neg = true;
          ch = ch->neg();
        }
        auto map = dynamic_cast<Mapping*>(ch);
        auto var = dynamic_cast<Variable*>(ch);
        assert(map || var);
        // printf("ADD edge %i %i for %s\n", node_ids[c], neg + (map ? var_ids[map->getValue(params)]
        //                                      : var_ids[var_ids[var->id()]]), c->pretty().c_str());
        g->add_edge(node_ids[c], neg + (map ? var_ids[map->getValue(params)]
                                            : var_ids[var_ids[var->id()]]));
      } else {
        // printf("add edge %i %i for %s to %s.\n", node_ids[c], node_ids[ch], c->pretty().c_str(), ch->pretty().c_str());
        g->add_edge(node_ids[c], node_ids[ch]);
        AddEdges(ch);
      }
    }
  };
  for (auto outcome: outcomes_) {
    AddEdges(outcome);
  }
  g->set_splitting_heuristic(bliss::Digraph::shs_fsm);
  return g;
}
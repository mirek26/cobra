/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include <exception>
#include <cassert>

#include "common.h"
#include "formula.h"
#include "game.h"

#include "experiment.h"

void Experiment::addOutcome(std::string name, Formula* outcome) {
  outcomes_names_.push_back(name);
  outcomes_.push_back(outcome);
}

void Experiment::paramsDistinct(std::vector<uint>* list) {
  for(auto i = list->begin(); i != list->end(); ++i) {
    for(auto j = i + 1; j != list->end(); ++j) {
      assert(*i < *j && *j - 1 < num_params_);
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
      assert(*i < *j && *j - 1 < num_params_);
      // params are internally indexed from 0, that's why *i - 1
      params_smaller_than_[*j - 1].insert(*i - 1);
    }
  }
  delete list;
}

void Experiment::PrecomputeUsed(Construct* f) {
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
        if (used_vars_.count(var) > 1)
          interchangable_[d][a] = false;
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
      printf("%s %i %i -> %s\n", name_.c_str(), d, a, interchangable_[d][a] ? "true" : "false");
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
      for (auto p: tmp_params_) printf("%i ", p);
      printf(".\n");
    } else {
      // for (int i = 0; i <= n; i++) printf("%i ", tmp_params_[i]);
      // printf("...\n");
      FillParametrization(groups, n + 1);
    }
  }
}

void Experiment::GenerateParametrizations(std::vector<int> groups) {
  tmp_params_.resize(num_params_);
  tmp_params_all_.clear();
  FillParametrization(groups, 0);
}
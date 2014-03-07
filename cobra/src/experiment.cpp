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
  uint alph = game_->alphabet().size();
  printf("alph = %i\n", alph);
  interchangable_.resize(num_params_);
  for (uint d = 0; d < num_params_; d++) {
    interchangable_[d].resize(alph);
    for (CharId a = 0; a < alph; a++) {
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
        for (CharId b = 0; b < alph; b++) {
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

void Experiment::GenerateParametrizations(/*std::vector<int> equiv?*/) {


}
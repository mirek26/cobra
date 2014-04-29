/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <cmath>
#include <vector>
#include <map>
#include <exception>
#include "common.h"
#include "formula.h"
#include "game.h"
#include <bliss/graph.hh>

#ifndef COBRA_EXPERIMENT_H_
#define COBRA_EXPERIMENT_H_

struct GenParamsStats {
  uint ph1 = 0, ph2 = 0, ph3 = 0;
};

struct Outcome {
  string name;
  Formula* formula;
  bool last;
};

class Option {
  Solver& solver_;
  Experiment& type_;
  vec<CharId> params_;
  uint index_;

  vec<bool> sat_;
  uint sat_num_;
  vec<uint> models_;
  uint models_total_;
  vec<uint> fixed_;

 public:
  Option(Solver& solver, Experiment& e, vec<CharId> params, uint index):
    solver_(solver),
    type_(e),
    params_(params),
    index_(index) { }

  Experiment& type() const { return type_; }
  const vec<CharId>& params() const { return params_; }
  uint index() const { return index_; }

  uint GetNumOfSatOutcomes() {
    if (sat_.empty()) ComputeSat();
    return sat_num_;
  }

  bool IsOutcomeSat(uint id) {
    if (sat_.empty()) ComputeSat();
    assert(id < sat_.size());
    return sat_[id];
  }

  vec<uint>& GetNumOfModels() {
    if (models_.empty()) ComputeNumOfModels();
    return models_;
  }

  uint GetTotalNumOfModels() {
    if (models_.empty()) ComputeNumOfModels();
    return models_total_;
  }

  vec<uint>& GetNumOfFixedVars() {
    if (fixed_.empty()) ComputeFixedVars();
    return fixed_;
  }

 private:
  void ComputeSat();
  void ComputeNumOfModels();
  void ComputeFixedVars();
};


class Experiment {
  Game& game_;
  uint alph_;

  string name_;
  uint num_params_;

  uint final_outcome_;
  vec<Outcome> outcomes_;

  vec<set<MapId>> used_maps_;
  set<VarId> used_vars_;
  vec<vec<bool>> interchangable_;

  vec<set<uint>> params_different_;
  vec<set<uint>> params_smaller_than_;

  // Helper fields for parametrization generation.
  GenParamsStats gen_stats_;
  vec<uint> gen_var_groups_;
  set<vec<CharId>> gen_params_basic_;
  set<vec<CharId>> gen_params_final_;
  vec<CharId> gen_params_;
  std::map<unsigned int, vec<CharId>> gen_graphs_;

 public:
  Experiment(Game& game, string name, uint num_params):
      game_(game),
      name_(name),
      num_params_(num_params),
      final_outcome_(-1) {
    params_different_.resize(num_params);
    params_smaller_than_.resize(num_params);
    alph_ = game_.alphabet().size();
  }

  string name() const { return name_; }
  Game& game() const { return game_; }

  uint final_outcome() const { return final_outcome_; }
  const vec<Outcome>& outcomes() const { return outcomes_; }

  uint num_params() { return num_params_; }

  // Functions defining the experiment.
  void addOutcome(string name, Formula* outcome, bool final = true);
  void paramsDistinct(vec<uint>* list);
  void paramsSorted(vec<uint>* list);

  set<vec<CharId>>& GenParams(vec<uint>&);
  void Precompute();

  uint64_t NumberOfParametrizations() const {
    uint64_t total = 1;
    for (uint i = 0; i < num_params_; i++) {
      int pos = alph_;
      for (auto p: params_different_[i]) {
        if (p < i) pos--;
      }
      total *= pos;
    }
    return total;
  }

 private:
  // Computes used_maps_ and used_vars_.
  void PrecomputeUsed(Formula* f);

  // Helper functions for parametrizations generation.
  bool CharsEquiv(set<MapId>& maps, CharId a, CharId b) const;
  void GenParamsFill(uint n);
  void GenParamsBasicFilter();
  void GenParamsGraphFilter();

  bliss::Digraph* CreateGraphForParams(vec<uint>& groups,
                                       vec<CharId>& params);

};

#endif  // COBRA_EXPERIMENT_H_
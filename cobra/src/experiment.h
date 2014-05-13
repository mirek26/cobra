/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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

struct EvalExp;

struct GenParamsStats {
  uint ph1 = 0, ph2 = 0, ph3 = 0;
};

struct Outcome {
  string name;
  Formula* formula;
  bool last;
};

struct EOutcome {
  bool sat_c = false, models_c = false, fixed_c = false;
  bool sat;
  int models;
  int fixed;
};

class Experiment {
  Solver& solver_;
  ExpType& type_;
  vec<CharId> params_;
  uint index_;

  vec<EOutcome> data_;

 public:
  Experiment(Solver& solver, ExpType& e, vec<CharId> params, uint index);

  ExpType& type() const { return type_; }
  const vec<CharId>& params() const { return params_; }
  uint index() const { return index_; }
  uint num_outcomes() const { return data_.size(); }

  bool IsSat(uint id);
  uint NumOfSat();
  uint NumOfModels(uint id);
  uint TotalNumOfModels();
  uint NumOfFixedVars(uint id);
};


class ExpType {
  Game& game_;
  uint alph_;

  string name_;
  uint num_params_;

  int final_outcome_;
  vec<Outcome> outcomes_;

 public:
  vec<set<MapId>> used_maps_;
  set<VarId> used_vars_;
  vec<vec<bool>> interchangable_;

  vec<set<uint>> params_different_;
  vec<set<uint>> params_smaller_than_;

 public:
  ExpType(Game& game, string name, uint num_params);

  string name() const { return name_; }
  Game& game() const { return game_; }
  int final_outcome() const { return final_outcome_; }
  const vec<Outcome>& outcomes() const { return outcomes_; }
  uint num_params() { return num_params_; }

  // Functions defining the experiment.
  void addOutcome(string name, Formula* outcome, bool final = true);
  void paramsDistinct(vec<uint>* list);
  void paramsSorted(vec<uint>* list);

  set<vec<CharId>>& GenParams(vec<uint>&);
  void Precompute();

  uint64_t NumberOfParametrizations() const;
  bliss::Graph* CreateGraphForParams(const vec<EvalExp>& history,
                                     const vec<CharId>& params) const;

 private:
  // Computes used_maps_ and used_vars_.
  void PrecomputeUsed(Formula* f);
};

struct EvalExp {
  Experiment exp;
  uint outcome_id;
};


class ExpGenerator {
  Game& game_;
  Solver& solver_;

  vec<VarId> fixed_vars_;
  std::map<unsigned int, std::pair<vec<CharId>, bool>> graphs_;

  GenParamsStats stats_;
  bliss::Graph* graph_;
  vec<uint> var_groups_;

  ExpType* curr_type_;

  set<vec<CharId>> params_basic_;
  set<vec<CharId>> params_final_;
  vec<CharId> params_;

 public:
  ExpGenerator(Game& game, Solver& solver, const vec<EvalExp>& history);
  ~ExpGenerator() {
    delete graph_;
  }

  // TODO: incremental generation
  // Experiment Next();
  vec<Experiment> All();

 private:
  // Helper functions for parametrizations generation.
  bool CharsEquiv(set<MapId>& maps, CharId a, CharId b) const;
  void GenParamsFill(uint n);
  void GenParamsBasicFilter();
  void GenParamsGraphFilter();
};


#endif  // COBRA_EXPERIMENT_H_
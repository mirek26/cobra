/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cassert>
#include <cmath>
#include <exception>
#include <unordered_map>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <bliss/graph.hh>
#include "./common.h"
#include "./formula.h"
#include "./game.h"

#ifndef COBRA_SRC_EXPERIMENT_H_
#define COBRA_SRC_EXPERIMENT_H_

struct EvalExp;

struct GenParamsStats {
  uint ph1 = 0, ph2 = 0, ph3 = 0;
};

struct Outcome {
  string name;
  Formula* formula;
  bool final;
};

struct EOutcome {
  bool sat_c = false, models_c = false, fixed_c = false;
  bool sat;
  int models;
  int fixed;
};

class Experiment {
  Solver* solver_;
  const ExpType* type_;
  vec<CharId> params_;
  uint index_;

  vec<EOutcome> data_;

 public:
  Experiment(Solver& solver, const ExpType& e,
             vec<CharId> params, uint index);

  const ExpType& type() const { return *type_; }
  const vec<CharId>& params() const { return params_; }
  uint index() const { return index_; }
  uint num_outcomes() const { return data_.size(); }

  bool IsFinalSat();
  bool IsSat(uint id);
  uint NumOfSat();
  uint NumOfModels(uint id);
  uint TotalNumOfModels();
  uint MaxNumOfModels();
  uint NumOfFixedVars(uint id);

  string pretty();
};


class ExpType {
  const Game& game_;
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
  ExpType(const Game& game, string name, uint num_params);

  string name() const { return name_; }
  const Game& game() const { return game_; }
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


class GraphHash {
public:
  size_t operator()(bliss::Graph* g) const {
    return g->get_hash();
  }
};

struct GraphEquals : std::binary_function<bliss::Graph*, bliss::Graph*, bool> {
  result_type operator()(first_argument_type lhs, second_argument_type rhs) const {
    return lhs->cmp(*rhs) == 0;
  }
};

class ExpGenerator {
  const Game& game_;
  Solver& solver_;
  bool use_bliss_;

  vec<VarId> fixed_vars_;

  std::unordered_map<bliss::Graph*, uint, GraphHash, GraphEquals> graphs_;

  GenParamsStats stats_;
  bliss::Graph* graph_;
  vec<uint> var_groups_;

  ExpType* curr_type_;

  //set<vec<CharId>> params_basic_;
  vec<CharId> params_;

  vec<Experiment> experiments_;

 public:
  ExpGenerator(const Game& game, Solver& solver, const vec<EvalExp>& history,
               bool use_bliss);
  ~ExpGenerator();

  // TODO: incremental generation
  // Experiment Next();
  vec<Experiment> All();
  bliss::Graph* graph() const { return graph_; }

 private:
  // Helper functions for parametrizations generation.
  bool CharsEquiv(const set<MapId>& maps, CharId a, CharId b) const;
  void GenParamsFill(uint n);
  void GenParamsBasicFilter();
  void GenParamsGraphFilter();
};

#endif  // COBRA_SRC_EXPERIMENT_H_

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

  vec<set<MapId>> used_maps_;
  set<VarId> used_vars_;
  vec<vec<bool>> interchangable_;

  vec<set<uint>> params_different_;
  vec<set<uint>> params_smaller_than_;

  GenParamsStats gen_stats_;
  vec<uint> gen_var_groups_;

  set<vec<CharId>> gen_params_basic_;
  set<vec<CharId>> gen_params_final_;
  vec<CharId> gen_params_;
  std::map<unsigned int, vec<CharId>> gen_graphs_;

 public:
  ExpType(Game& game, string name, uint num_params):
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

  int final_outcome() const { return final_outcome_; }
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

struct EvalExp {
  Experiment exp;
  uint outcome_id;
};


class ExpGenerator {
  Game& game_;
  Solver& solver_;
  const vec<EvalExp>& history_;

  std::map<unsigned int, vec<CharId>> graphs_;

  GenParamsStats stats_;
  vec<uint> var_groups_;
  int curr_tid;

  set<vec<CharId>> params_basic_;
  set<vec<CharId>> params_final_;
  vec<CharId> params_;

 public:
  ExpGenerator(Game& game, Solver& solver, const vec<EvalExp>& history):
    game_(game),
    solver_(solver),
    history_(history) {
    stats_ = GenParamsStats();
    // prepare var_groups_
    auto graph = game_.CreateGraph();
    game_.restriction()->AddToGraph(*graph, nullptr);
    for (auto& e: history_) {
      e.exp.type().outcomes()[e.outcome_id].formula->AddToGraph(*graph, &e.exp.params());
    }
    var_groups_ = game_.ComputeVarEquiv(solver_, *graph);

    curr_tid = 0;
  }

  // Experiment Next();

  vec<Experiment> All() {
    vec<Experiment> result;
    for (auto t: game_.experiments()) {
      auto& params_all = t->GenParams(var_groups_);
      for (auto& params: params_all) {
        result.push_back(Experiment(solver_, *t, params, result.size()));
      }
    }
    return result;
  }
};


#endif  // COBRA_EXPERIMENT_H_
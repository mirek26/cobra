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

  bliss::Graph* CreateGraphForParams(const vec<EvalExp>& history,
                                       const vec<CharId>& params);

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
  const vec<EvalExp>& history_;

  vec<VarId> fixed_vars_;
  std::map<unsigned int, vec<CharId>> graphs_;

  GenParamsStats stats_;
  bliss::Graph* graph_;
  vec<uint> var_groups_;

  uint curr_tid;
  ExpType* curr_type_;

  set<vec<CharId>> params_basic_;
  set<vec<CharId>> params_final_;
  vec<CharId> params_;

 public:
  ExpGenerator(Game& game, Solver& solver, const vec<EvalExp>& history):
    game_(game),
    solver_(solver),
    history_(history) {
    stats_ = GenParamsStats();
    // Preprare symmetry Graph
    graph_ = game.CreateGraph();
    fixed_vars_ = solver.GetFixedVars();
    game.restriction()->PropagateFixed(fixed_vars_, nullptr);
    game.restriction()->AddToGraph(*graph_, nullptr, vertex_type::kKnowledgeRoot);
    for (auto& e: history) {
      auto formula = e.exp.type().outcomes()[e.outcome_id].formula;
      formula->PropagateFixed(fixed_vars_, &e.exp.params());
      formula->AddToGraph(*graph_, &e.exp.params());
    }

    for (auto e: game.experiments()) {
      for (auto& maps: e->used_maps_) {
        for (auto v: maps) for (auto u: maps) {
          if (v == u) continue;
          for (uint i = 0; i < game.alphabet().size(); i++) {
            auto v1 = game.getMappingValue(u, i);
            auto v2 = game.getMappingValue(v, i);
            graph_->add_edge(2*v1 - 2, 2*v2 - 2);
          }
        }
      }
    }

    var_groups_ = game_.ComputeVarEquiv(solver_, *graph_);
  }

  ~ExpGenerator() {
    delete graph_;
  }

  // Experiment Next();

  vec<Experiment> All() {
    vec<Experiment> result;
    for (auto t: game_.experiments()) {
      curr_type_ = t;

      params_.resize(t->num_params());
      params_basic_.clear();
      params_final_.clear();
      auto pre2 = stats_.ph2;
      auto pre3 = stats_.ph3;

      GenParamsFill(0);

      //printf("=== Gen params stats: %i %i %i.\n", gen_stats_.ph1, gen_stats_.ph2, gen_stats_.ph3);
      assert(stats_.ph2 - pre2 == params_basic_.size());
      assert(stats_.ph3 - pre3 == params_final_.size());

      for (auto& params: params_final_) {
        result.push_back(Experiment(solver_, *t, params, result.size()));
      }
    }
    return result;
  }

 private:
  // Helper functions for parametrizations generation.
  bool CharsEquiv(set<MapId>& maps, CharId a, CharId b) const;
  void GenParamsFill(uint n);
  void GenParamsBasicFilter();
  void GenParamsGraphFilter();
};


#endif  // COBRA_EXPERIMENT_H_
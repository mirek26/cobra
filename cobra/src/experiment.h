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
#include <bliss-0.72/graph.hh>

#ifndef COBRA_EXPERIMENT_H_
#define COBRA_EXPERIMENT_H_

class Experiment {
  Game* game_;
  uint alph_;

  std::string name_;
  uint num_params_;

  std::vector<std::string> outcomes_names_;
  std::vector<Formula*> outcomes_;
  std::vector<CnfFormula> outcomes_cnf_;

  std::vector<std::set<MapId>> used_maps_;
  std::set<VarId> used_vars_;
  std::vector<std::vector<bool>> interchangable_;

  std::vector<std::set<uint>> params_different_;
  std::vector<std::set<uint>> params_smaller_than_;

  std::set<std::vector<CharId>> tmp_params_all_;
  std::vector<CharId> tmp_params_;

 public:
  Experiment(Game* game, std::string name, uint num_params):
      game_(game),
      name_(name),
      num_params_(num_params) {
    params_different_.resize(num_params);
    params_smaller_than_.resize(num_params);
    alph_ = game_->alphabet().size();
  }

  std::string name() const { return name_; }
  const std::vector<Formula*>& outcomes() const { return outcomes_; }

  // Functions defining the experiment.
  void addOutcome(std::string name, Formula* outcome);
  void paramsDistinct(std::vector<uint>* list);
  void paramsSorted(std::vector<uint>* list);

  void GenerateParametrizations(std::vector<int> symmetry);
  void Precompute();

 private:
  // Computes used_maps_ and used_vars_.
  void PrecomputeUsed(Formula* f);

  // Helper functions for parametrizations generation.
  bool CharsEquivalent(uint n, CharId a, CharId b, std::vector<int>& groups) const;
  void FillParametrization(std::vector<int>& groups, uint n);

  bliss::Digraph* BlissGraphForParametrization(
                          std::vector<int>& groups,
                          std::vector<CharId>& params);

};

#endif  // COBRA_EXPERIMENT_H_
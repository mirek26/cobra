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

#ifndef COBRA_EXPERIMENT_H_
#define COBRA_EXPERIMENT_H_

class Experiment {
  Game* game_;

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

 public:
  Experiment(Game* game, std::string name, uint num_params):
    game_(game),
    name_(name),
    num_params_(num_params) {
    params_different_.resize(num_params);
    params_smaller_than_.resize(num_params);
  }

  std::string name() {
    return name_;
  }

  void GenerateParametrizations();

  void addOutcome(std::string name, Formula* outcome);

  void paramsDistinct(std::vector<uint>* list);
  void paramsSorted(std::vector<uint>* list);

  void Precompute();

 private:
  void PrecomputeUsed(Construct* f);
};

#endif  // COBRA_EXPERIMENT_H_
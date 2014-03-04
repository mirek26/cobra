/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include <exception>
#include <cassert>
#include "util.h"
#include "formula.h"
#include "game.h"

#ifndef COBRA_EXPERIMENT_H_
#define COBRA_EXPERIMENT_H_

class Experiment {
  std::string name_;
  int num_params_;

  std::vector<std::string> outcomes_names_;
  FormulaList* outcomes_;
  std::vector<CnfFormula> outcomes_cnf_;

 public:
  Experiment(std::string name, int num_params):
    name_(name),
    num_params_(num_params) { }

  std::string name() {
    return name_;
  }

  void addOutcome(std::string name, Formula* outcome);

  void paramsDistinct(std::vector<int>* list);
  void paramsSorted(std::vector<int>* list);

};

#endif  // COBRA_EXPERIMENT_H_
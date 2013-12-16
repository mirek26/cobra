/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include "util.h"
#include "cnf-formula.h"

#ifndef COBRA_GAME_H_
#define COBRA_GAME_H_

class Formula;
class VariableSet;
class Experiment;

class Game {
  VariableSet* variables_;
  Formula* init_;
  std::vector<Experiment*> experiments_;

 public:
  void setVariables(VariableSet* vars);

  Formula* init() {
    return init_;
  }

  void setInit(Formula* init) {
    init_ = init;
  }

  void addExperiment(Experiment* e) {
    experiments_.push_back(e);
  }

  std::vector<Experiment*>& experiments() {
    return experiments_;
  }

  bool complete() {
    if (!variables_) {
      printf("Nothing assigned to 'Vars'.\n");
      return false;
    }
    if (!init_) {
      printf("Nothing assigned to 'Init'.\n");
      return false;
    }
    if (experiments_.empty()) {
      printf("No experiment defined.\n");
      return false;
    }
    return true;
  }

  void doAll();
};

#endif   // COBRA_GAME_H_
/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include "common.h"
#include "cnf-formula.h"

#ifndef COBRA_GAME_H_
#define COBRA_GAME_H_

class Formula;
class VariableList;
class Experiment;

class Game {
  VariableList* variables_;
  Formula* init_;
  std::vector<Experiment*> experiments_;
  std::vector<std::string> alphabet_;

  std::vector<std::vector<int>> mappings_;
  std::map<std::string, int> mappings_ids_;

 public:
  void declareVariable(Variable*);
  void declareVariables(VariableList*);
// getVariableById?

  void addRestriction(Formula*);
  Formula* initialRestrictions();

  void setAlphabet(std::vector<std::string>* alphabet) {
    assert(alphabet_.empty());
    alphabet_.insert(alphabet_.end(), alphabet->begin(), alphabet->end());
    delete alphabet;
  }

  const std::vector<std::string>& alphabet() { return alphabet_; }

  int addMapping(std::string, VariableList*);
  int getMappingId(std::string);
  int getMappingValue(uint, uint);

  Experiment* addExperiment(std::string name, uint num_params);
  std::vector<Experiment*>& experiments() {  return experiments_;  }

  void Precompute();

  bool complete() {
    // if (!variables_) {
    //   printf("Nothing assigned to 'Vars'.\n");
    //   return false;
    // }
    // if (!init_) {
    //   printf("Nothing assigned to 'Init'.\n");
    //   return false;
    // }
    // if (experiments_.empty()) {
    //   printf("No experiment defined.\n");
    //   return false;
    // }
    return true;
  }

  void doAll();
};

#endif   // COBRA_GAME_H_
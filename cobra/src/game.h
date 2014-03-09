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
class Experiment;

class Game {
  std::vector<Variable*> variables_;
  Formula* init_;
  std::vector<Experiment*> experiments_;
  std::vector<std::string> alphabet_;

  // list of mappings between alphabet and variables
  std::vector<std::vector<VarId>> mappings_;
  std::map<std::string, MapId> mappings_ids_;

 public:
  void declareVariable(Variable*);
  void declareVariables(std::vector<Variable*>*);
  std::vector<Variable*>& variables() { return variables_; }
// getVariableById?

  void addRestriction(Formula*);
  Formula* initialRestrictions();

  void setAlphabet(std::vector<std::string>* alphabet) {
    assert(alphabet_.empty());
    alphabet_.insert(alphabet_.end(), alphabet->begin(), alphabet->end());
    delete alphabet;
  }

  const std::vector<std::string>& alphabet() { return alphabet_; }

  int addMapping(std::string, std::vector<Variable*>*);
  int getMappingId(std::string);
  int getMappingValue(MapId, CharId);

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
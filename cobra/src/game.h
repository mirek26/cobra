/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include <bliss/graph.hh>
#include "common.h"
#include "cnf-formula.h"

#ifndef COBRA_GAME_H_
#define COBRA_GAME_H_

class Formula;
class Experiment;
class AndOperator;

class Game {
  vec<Variable*> variables_;
  std::map<string, VarId> variables_ids_;
  AndOperator* restriction_;
  vec<Experiment*> experiments_;
  vec<string> alphabet_;

  // list of mappings between alphabet and variables
  vec<vec<VarId>> mappings_;
  std::map<string, MapId> mappings_ids_;

 public:
  Game();

  void declareVariable(Variable*);
  void declareVariables(vec<Variable*>*);
  void declareVariables(std::initializer_list<string>);
  vec<Variable*>& variables() { return variables_; }

  Variable* getVariableByName(string);

  void addRestriction(Formula*);
  Formula* restriction();

  void setAlphabet(vec<string>* alphabet) {
    assert(alphabet_.empty());
    alphabet_.insert(alphabet_.end(), alphabet->begin(), alphabet->end());
    delete alphabet;
  }

  const vec<string>& alphabet() { return alphabet_; }

  int addMapping(string, vec<Variable*>*);
  int getMappingId(string);
  int getMappingValue(MapId, CharId);

  Experiment* addExperiment(string name, uint num_params);
  vec<Experiment*>& experiments() {  return experiments_;  }

  void Precompute();

  bliss::Digraph* CreateGraph();
  vec<int> ComputeVarEquiv(bliss::Digraph&);
};

#endif   // COBRA_GAME_H_
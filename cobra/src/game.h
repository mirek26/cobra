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
class Option;

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

  MapId addMapping(string, vec<Variable*>*);
  MapId getMappingId(string);
  VarId getMappingValue(MapId, CharId);

  Experiment* addExperiment(string name, uint num_params);
  vec<Experiment*>& experiments() {  return experiments_;  }

  void printParams(const vec<CharId>& params){
    for (auto a: params)
      printf("%s ", alphabet_[a].c_str());
    printf("\b");
  }

  void Precompute();

  bliss::Digraph* CreateGraph();
  vec<uint> ComputeVarEquiv(CnfFormula& solver, bliss::Digraph& graph);
  vec<Option> GenerateExperiments(CnfFormula& solver, bliss::Digraph& graph);



};

#endif   // COBRA_GAME_H_
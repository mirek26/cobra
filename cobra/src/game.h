/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include <bliss/graph.hh>
#include "common.h"
#include "pico-solver.h"

#ifndef COBRA_GAME_H_
#define COBRA_GAME_H_

class Formula;
class Experiment;
class AndOperator;
class Option;

class Game {
  vec<Variable*> vars_;
  std::map<string, VarId> vars_ids_;
  AndOperator* restriction_;
  vec<Experiment*> experiments_;
  vec<string> alphabet_;

  uint bliss_calls_;
  clock_t bliss_time_;

  // list of mappings between alphabet and vars
  vec<vec<VarId>> mappings_;
  std::map<string, MapId> mappings_ids_;

 public:
  Game();

  void declareVar(Variable*);
  void declareVars(vec<Variable*>*);
  void declareVars(std::initializer_list<string>);
  vec<Variable*>& vars() { return vars_; }

  Variable* getVarByName(string);

  void addRestriction(Formula*);
  Formula* restriction();

  void setAlphabet(vec<string>* alphabet) {
    assert(alphabet_.empty());
    alphabet_.insert(alphabet_.end(), alphabet->begin(), alphabet->end());
    delete alphabet;
  }

  const vec<string>& alphabet() { return alphabet_; }

  uint& bliss_calls() { return bliss_calls_; }
  clock_t& bliss_time() { return bliss_time_; }

  MapId addMapping(string, vec<Variable*>*);
  MapId getMappingId(string);
  VarId getMappingValue(MapId, CharId);

  Experiment* addExperiment(string name, uint num_params);
  vec<Experiment*>& experiments() {  return experiments_;  }

  void PrintCode(vec<bool> code);

  string ParamsToStr(const vec<CharId>& params, char sep = ' '){
    string s = "";
    for (auto a: params)
      s += alphabet_[a] + sep;
    s.erase(s.length()-1, 1);
    return s;
  }

  void Precompute();

  bliss::Digraph* CreateGraph();
  vec<uint> ComputeVarEquiv(Solver& solver, bliss::Digraph& graph);
  vec<Option> GenerateExperiments(Solver& solver, bliss::Digraph& graph);



};

#endif   // COBRA_GAME_H_
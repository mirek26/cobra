/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cassert>
#include <vector>
#include <map>
#include <string>
#include <bliss/graph.hh>
#include "./common.h"
#include "./picosolver.h"

#ifndef COBRA_SRC_GAME_H_
#define COBRA_SRC_GAME_H_

class Formula;
class ExpType;
class AndOperator;
class Experiment;

/**
 * Class for representation of a code-breaking game.
 */
class Game {
  vec<Variable*> vars_;
  std::map<string, VarId> vars_ids_;
  AndOperator* constraint_;
  vec<ExpType*> experiments_;
  vec<string> alphabet_;

  /**
   * Collection of mappings between alphabet and vars
   */
  vec<vec<VarId>> mappings_;
  std::map<string, MapId> mappings_ids_;

 public:
  static uint bliss_calls;
  static clock_t bliss_time;

  Game();

  void declareVar(Variable* var);
  void declareVars(vec<Variable*>* list);
  void declareVars(std::initializer_list<string> list);
  const vec<Variable*>& vars() const { return vars_; }

  Variable* getVarByName(string) const;

  Formula* constraint() const;
  void addConstraint(Formula* f);

  const vec<string>& alphabet() const { return alphabet_; }
  void setAlphabet(vec<string>* alphabet) {
    assert(alphabet_.empty());
    alphabet_.insert(alphabet_.end(), alphabet->begin(), alphabet->end());
    delete alphabet;
  }

  MapId addMapping(string, vec<Variable*>*);
  MapId getMappingId(string) const;
  VarId getMappingValue(MapId, CharId) const;
  uint numMappings() const { return mappings_.size(); }
  ExpType* addExperiment(string name, uint num_params);
  const vec<ExpType*>& experiments() const {  return experiments_;  }

  void PrintModel(vec<bool> model) const;
  string ParamsToStr(const vec<CharId>& params, char sep = ' ') const;
  void Precompute();

  bliss::Graph* CreateGraph() const;
};

#endif  // COBRA_SRC_GAME_H_

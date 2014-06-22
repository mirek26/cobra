/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <set>
#include <string>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <bliss/graph.hh>
#include <bliss/utils.hh>
#include "./game.h"
#include "./experiment.h"
#include "./common.h"
#include "./solver.h"

extern Args args;

struct stateInfo {
  double opt;
  double bound;
  Experiment* exp;
  vec<int> next;
  bool solved = false;
  bool printed = false;
};

class OptimalGenerator {
  // a state represents partial information,
  // contains optimal number to finish the game and which experiment to choose
  std::vector<stateInfo> states_;

  // mapping from graphs to state ids
  std::unordered_map<bliss::Graph*, uint, GraphHash, GraphEquals> graph_hash_;

  Solver& solver_;
  Game& game_;
  bool worst_;
  uint init_;
  vec<EvalExp> history_;

 public:
  OptimalGenerator(Solver& solver, Game& game, bool worst, double opt_bound)
    : solver_(solver), game_(game), worst_(worst) {
    init_ = GetCurrentState(opt_bound);
    verify(init_);
  }

  bool success() { return states_[init_].exp != nullptr; }
  double value() { return states_[init_].opt; }

  ~OptimalGenerator() {
    // Free aux structures
    // printf("Destructor.\n");
    for (auto g: graph_hash_) delete g.first;
    for (auto& x: states_)
      if (x.exp) delete x.exp;
    // printf("Done.\n");
  }

 private:
  int InitNewState(double bound);

  // Attempts to find the current state (given by state of solver_ and history_)
  // in the hash, if it is not present, computes the optimal strategy for the state.
  // Returns the id of the state in the states_ vector.
  int GetCurrentState(double bound);

  void Compute(ExpGenerator& gen, uint id, double bound);
  double AnalyzeExperiment(Experiment& e, vec<int>& next, double bound, uint models, uint maxparts);

  // Goes through all experiments, filters out experiments with only possible
  // outcome; if it identifies an experiment that directly solves the game,
  // sets the corresponding information to the state and returns true.
  bool FilterOptions(uint id, vec<Experiment>& options,
                     uint models, uint* maxparts);
  void MarkAsFinished(uint id);

  void verify(uint id);
};

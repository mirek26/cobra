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
#include "./simple-solver.h"
#include "./optimal.h"

int OptimalGenerator::InitNewState(double bound) {
  states_.push_back(stateInfo());
  auto id = states_.size() - 1;
  states_[id].opt = -1;
  states_[id].bound = bound;
  states_[id].exp = nullptr;
  return id;
}

// Attempts to find the current state (given by state of solver_ and history_)
// in the hash, if it is not present, computes the optimal strategy for the state.
// Returns the id of the state in the states_ vector.
int OptimalGenerator::GetCurrentState(double bound) {
  ExpGenerator gen(game_, solver_, history_, args.symmetry_detection);
  auto graph = gen.graph();
  bliss::Stats stats;
  clock_t t1 = clock();
  // auto canonical = new bliss::Graph(*graph);
  auto canonical = graph->permute(graph->canonical_form(stats, nullptr, nullptr));
  Game::bliss_calls += 1;
  Game::bliss_time += clock() - t1;

  // if the value for this subproblem is already computed, simply return
  if (graph_hash_.count(canonical)) {
    // printf("Cache hit.\n");
    auto id = graph_hash_[canonical];
    delete canonical;
    if (states_[id].exp == nullptr && states_[id].bound < bound) {
      // printf("Recompute (old bound %.2f, new %.2f)\n", states_[id].opt, bound);
      states_[id].bound = bound;
      Compute(gen, id, bound);
    }
    return id;
  }

  auto id = InitNewState(bound);
  graph_hash_[canonical] = id;
  Compute(gen, id, bound);
  return id;
}

void OptimalGenerator::Compute(ExpGenerator& gen, uint id, double bound) {
  auto& simp = static_cast<SimpleSolver&>(solver_);
  string formula = simp.pretty();
  // printf("Formula %s. Compute with bound %.2f\n", formula.c_str(), bound);
  if (solver_.OnlyOneModel()) {
    MarkAsFinished(id);
    // printf("Formula %s - only one model (%.2f).\n", formula.c_str(), states_[id].opt);
    return;
  }

  vec<Experiment> options = gen.All();
  auto models = solver_.NumOfModels();
  uint maxparts;
  if (FilterOptions(id, options, models, &maxparts)) {
    // printf("Formula %s - can finish in one exp (%.2f).\n", formula.c_str(), states_[id].opt);
    return;
  }

  // Sort according to the 'max-models' strategy
  assert(!options.empty());
  vec<uint> sorted;
  sorted.resize(options.size());
  for (uint i = 0; i < options.size(); i++) sorted[i] = i;
  std::sort(sorted.begin(), sorted.end(), [&](const int a, const int b) {
    return options[a].MaxNumOfModels() < options[b].MaxNumOfModels();
  });

  // Analyze all options
  int best = -1;
  for (auto i : sorted) {
    auto& e = options[i];
    vec<int> next(e.type().outcomes().size(), -1);
    auto val = AnalyzeExperiment(e, next, bound, models, maxparts);
    // printf("Formula %s - Experiment %s >> %.2f (bound %.2f)\n", formula.c_str(), e.pretty().c_str(), val, bound);
    if (val < bound) {
      bound = val;
      best = i;
      states_[id].next.resize(next.size());
      std::copy(next.begin(), next.end(), states_[id].next.begin());
    }
  }
  if (best > -1) states_[id].exp = new Experiment(options[best]);
  states_[id].opt = bound;
  // printf("Formula %s - normal end (%.2f)\n", formula.c_str(), bound);
}

double OptimalGenerator::AnalyzeExperiment(Experiment& e, vec<int>& next, double bound, uint models, uint maxparts) {
  // Lower bound on the value, progressively improving
  // For worst-case, the value of the worst outcome found so far
  // For average-case, the sum of trivial lower-bounds/values for outcomes
  double val = 0;

  // Lower bound check
  vec<double> lb(e.type().outcomes().size(), 0);
  for (uint i = 0; i < e.type().outcomes().size(); i++) {
    double imodels = static_cast<double>(e.NumOfModels(i));
    double ibound = 1 + (imodels > 1 ? log(imodels)/log(maxparts) : 0);
    if (worst_ && ceil(ibound) >= bound) return bound;
    if (!worst_) {
      lb[i] = ibound;
      val += imodels/models * lb[i];
    }
    //printf("%2.f\n", val);
  }
  if (!worst_ && val >= bound) return bound;

  // Try all outcomes
  for (uint i = 0; i < e.type().outcomes().size(); i++) {
    if (!e.IsSat(i)) continue;
    solver_.OpenContext();
    auto outcome = e.type().outcomes()[i];
    solver_.AddConstraint(outcome.formula, e.params());
    history_.push_back({ e, i });
    // compute upper bound for subproblem
    double nbound;
    if (worst_) {
      nbound = bound - 1;
    } else {
      // Compute nbound so that
      // val + imodels/models * (nbound - lb[i]) = bound
      nbound = (bound - val) * models / e.NumOfModels(i) + lb[i] - 1;
    }
    auto subproblem = GetCurrentState(nbound);
    assert(next.size() > i);
    next[i] = subproblem;
    if (worst_) {
      val = std::max(val, 1 + states_[subproblem].opt);
    } else {
      val += (1 + states_[subproblem].opt - lb[i]) * e.NumOfModels(i) / models;
      lb[i] = 1 + states_[subproblem].opt;
    }
    history_.pop_back();
    solver_.CloseContext();
    // if val reached the bound OR the subproblem couldn't be done in nbound
    if (val >= bound || (!states_[subproblem].exp && !states_[subproblem].solved)) {
      // printf("%.2f %.2f\n", val, bound);
      return bound;
    }
  }
  return val;
}


// Goes through all experiments, filters out experiments with only possible
// outcome; if it identifies an experiment that directly solves the game,
// sets the corresponding information to the state and returns true.
bool OptimalGenerator::FilterOptions(uint id, vec<Experiment>& options,
                                     uint models, uint* maxparts) {
  int finish = -1;
  *maxparts = 0;
  for (uint i = 0; i < options.size(); i++) {
    auto parts = options[i].NumOfSat();
    *maxparts = std::max(*maxparts, parts);
    if (parts == models) {  // this solves the game -> check for FINAL
      finish = i;
      auto final_out = options[i].type().final_outcome();
      if (final_out == -1 || options[i].IsSat(final_out)) break; // wait for one with IsSat(final)
    }
    if (parts == 1) {  // let out experiments with only one sat outcome
      options[i] = options.back();
      options.pop_back();
      i--;
    }
  }
  if (finish == -1) return false;
  states_[id].exp = new Experiment(options[finish]);  // clone the experiment
  states_[id].solved = true;
  auto final_out = options[finish].type().final_outcome();
  if (final_out == -1) states_[id].opt = 1;
  else if (worst_ || !options[finish].IsSat(final_out)) states_[id].opt = 2;
  else states_[id].opt = 2 - (static_cast<double>(1) / models);
  return true;
}

void OptimalGenerator::MarkAsFinished(uint id) {
  assert(!history_.empty());
  states_[id].solved = true;
  auto last = history_.back();
  auto final_out = last.exp.type().final_outcome();
  if (final_out == -1 || static_cast<int>(last.outcome_id) == final_out) {
    states_[id].opt = 0;
  } else {  // "Mastermind" anomaly
    states_[id].opt = 1;
  }
}

void OptimalGenerator::verify(uint id) {
  assert(id < states_.size());
  if (states_[id].printed) return;
  states_[id].printed = true;
  if (states_[id].solved) {
    if (states_[id].exp) printf("State %i: %s (Solved.)\n", id, states_[id].exp->pretty().c_str());
    else printf("State %i: Solved.\n", id);
    return;
  }
  assert(states_[id].exp);
  printf("State %i: %s. Go to ", id, states_[id].exp->pretty().c_str());
  for (auto n: states_[id].next) {
    if (n > -1) printf("%i ", n);
    else printf("-");
  }
  printf("\n");
  for (auto n: states_[id].next) {
    if (n > -1) verify(n);
  }
}

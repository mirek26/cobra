/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cmath>
#include "formula.h"
#include "game.h"
#include "experiment.h"
#include "common.h"
#include "parser.h"

#include "strategy.h"

namespace strategy {

namespace {
  // Helper function to select the experiment e with the largest f(e).
  // First parameter of f is the experiment, second is the value of the best option found yet.
  // This can save significant amount of time if the function evaluation
  // is time demanind (because of model counting, for example).
  uint maximize(std::function<double(Experiment&, double)> f, vec<Experiment>& options) {
    double max = -std::numeric_limits<double>::max();
    vec<Experiment>::iterator best = options.end();
    for (auto it = options.begin(); it != options.end(); ++it) {
      auto cand = f(*it, max);
      if (cand > max) {
        max = cand;
        best = it;
      }
    }
    assert(best != options.end());
    return std::distance(options.begin(), best);
  }

  // Similar to the previous function, just minimizing value of f.
  uint minimize(std::function<double(Experiment&, double)> f, vec<Experiment>& options) {
    double min = std::numeric_limits<double>::max();
    vec<Experiment>::iterator best = options.end();
    for (auto it = options.begin(); it != options.end(); ++it) {
      auto cand = f(*it, min);
      if (cand < min) {
        min = cand;
        best = it;
      }
    }
    assert(best != options.end());
    return std::distance(options.begin(), best);
  }
}

uint breaker::interactive(vec<Experiment>& options) {
  // Print options.
  printf("Select an experiment: \n");//%i: \n", exp_num++);
  for (auto& experiment: options) {
    if (experiment.NumOfSat() > 1) {
      printf("%i) %s [ %s ] ",
        experiment.index(),
        experiment.type().name().c_str(),
        experiment.type().game().ParamsToStr(experiment.params()).c_str());
      printf(" ] - M: ");
      for (uint i = 0; i < experiment.num_outcomes(); i++)
        printf("%i ", experiment.NumOfModels(i));
      printf("F: ");
      for (uint i = 0; i < experiment.num_outcomes(); i++)
        printf("%i ", experiment.NumOfFixedVars(i));
      printf("\n");
    }
  }

  // User prompt.
  uint choice;
  bool ok;
  string str;
  do {
    printf("> ");
    ok = readIntOrString(choice, str);
  } while (!ok || choice >= options.size() ||
           options[choice].NumOfSat() == 1);
  return choice;
}

uint maker::interactive(Experiment& option) {
  // Print options.
  printf("Select an outcome: \n");
  auto& type = option.type();
  for (uint i = 0; i < type.outcomes().size(); i++) {
    if (option.IsSat(i)) printf("%i) ", i);
    else printf("-) ");
    printf("%s %s\n",
      type.outcomes()[i].name.c_str(),
      option.IsSat(i) ? "" : "(unsatisfiable)");
  }

  // User prompt.
  uint choice;
  bool ok;
  string str;
  do {
    printf("> ");
    ok = readIntOrString(choice, str);
  } while (!ok || choice >= type.outcomes().size() ||
           !option.IsSat(choice));
  return choice;
}

uint breaker::random(vec<Experiment>& options) {
  return rand() % options.size();
}

uint maker::random(Experiment& option) {
  // Print options.
  int p = rand() % option.NumOfSat();
  int i = -1;
  while (p >= 0) {
    i++;
    if (option.IsSat(i)) p--;
  }
  return i;
}

uint maker::max_num(Experiment& option) {
  auto max = option.NumOfModels(0);
  auto best = 0;
  for (uint i = 1; i < option.num_outcomes(); i++) {
    auto cand = option.NumOfModels(i);
    if (cand > max) {
      max = cand;
      best = i;
    }
  }
  return best;
}

uint maker::fixed(Experiment& option) {
  auto min = option.NumOfFixedVars(0);
  auto best = 0;
  for (uint i = 1; i < option.num_outcomes(); i++) {
    auto cand = option.NumOfFixedVars(i);
    if (cand > min) {
      min = cand;
      best = i;
    }
  }
  return best;
}

uint breaker::min_num(vec<Experiment>& options) {
  return minimize([](Experiment& o, double min)->double {
    uint max = 0;
    for (uint i = 0; i < o.num_outcomes(); i++) {
      max = std::max(max, o.NumOfModels(i));
      if (max > min) return max;        // cannot have less than min
    }
    // prefer the experiment with a final outcome satisfiable
    auto f = o.type().final_outcome();
    if (f > -1 && o.IsSat(f)) return max - 0.5;
    return max;
  }, options);
}

uint breaker::exp_num(vec<Experiment>& options) {
  return minimize([](Experiment& o, double){
    int sumsq = 0;
    for (uint i = 0; i < o.num_outcomes(); i++) {
      auto models  = o.NumOfModels(i);
      sumsq += models * models;
    }
    return (double)sumsq/o.TotalNumOfModels();
  }, options);
}

uint breaker::entropy(vec<Experiment>& options) {
  return maximize([](Experiment& o, double){
    int total = o.TotalNumOfModels();
    double value = 0;
    for (uint i = 0; i < o.num_outcomes(); i++) {
      int models = o.NumOfModels(i);
      double p = (double) models/total;
      if (models > 0)
        value -= p * log2(p);
    }
    return value;
  }, options);
}

uint breaker::parts(vec<Experiment>& options) {
  return maximize([](Experiment& o, double max) ->double {
    uint sat = 0;
    for (uint i = 0; i < o.num_outcomes(); i++) {
      if (o.IsSat(i)) sat++;
      if (o.num_outcomes() - i + sat < max)
        return 0;        // cannot have more than max
    }
    // prefer the experiment with a final outcome satisfiable
    auto f = o.type().final_outcome();
    if (f > -1 && o.IsSat(f)) return sat + 0.5;
    return sat;
  }, options);
}

uint breaker::fixed(vec<Experiment>& options) {
  return maximize([](Experiment& o, double max) ->double {
    uint min = 0;
    for (uint i = 0; i < o.num_outcomes(); i++) {
      min = std::min(min, o.NumOfFixedVars(i));
      if (min < max) return min; // cannot have more than max
    }
    // prefer the experiment with a final outcome satisfiable
    auto f = o.type().final_outcome();
    if (f > -1 && o.IsSat(f)) return min + 0.5;
    return min;
  }, options);
}

}

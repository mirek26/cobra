/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <cmath>
#include "formula.h"
#include "game.h"
#include "experiment.h"
#include "common.h"
#include "parser.h"

#include "strategy.h"

uint Strategy::breaker_interactive(vec<Option>& options, Game& game) {
  // Print options.
  printf("Select an experiment: \n");//%i: \n", exp_num++);
  for (auto& experiment: options) {
    if (experiment.GetNumOfSatOutcomes() > 1) {
      printf("%i) %s [ ", experiment.index(), experiment.type().name().c_str());
      for (auto a: experiment.params())
        printf("%s ", game.alphabet()[a].c_str());
      printf("] - M: ");
      for (uint i = 0; i < experiment.type().outcomes().size(); i++)
        printf("%i ", experiment.GetNumOfModelsForOutcome(i));
      printf("F: ");
      for (uint i = 0; i < experiment.type().outcomes().size(); i++)
        printf("%i ", experiment.GetNumOfFixedVarsForOutcome(i));
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
           options[choice].GetNumOfSatOutcomes() == 1);
  return choice;
}

uint Strategy::maker_interactive(Option& option, Game&) {
  // Print options.
  printf("Select an outcome: \n");
  auto& type = option.type();
  auto& params = option.params();
  for (uint i = 0; i < type.outcomes().size(); i++) {
    if (option.IsOutcomeSat(i)) printf("%i) ", i);
    else printf("-) ");
    printf("%s - %s %s\n",
      type.outcomes_names()[i].c_str(),
      type.outcomes()[i]->pretty(true, &params).c_str(),
      option.IsOutcomeSat(i) ? "" : "(unsatisfiable)");
  }

  // User prompt.
  uint choice;
  bool ok;
  string str;
  do {
    printf("> ");
    ok = readIntOrString(choice, str);
  } while (!ok || choice >= type.outcomes().size() ||
           !option.IsOutcomeSat(choice));
  return choice;
}


uint Strategy::breaker_random(vec<Option>& options, Game&) {
  return rand() % options.size();
}

uint Strategy::maker_random(Option& option, Game&) {
  // Print options.
  int p = rand() % option.GetNumOfSatOutcomes();
  int i = -1;
  while (p >= 0) {
    i++;
    if (option.IsOutcomeSat(i)) p--;
  }
  return i;
}

uint Strategy::breaker_max(vec<Option>& options, Game&) {
  int result = 0;
  uint min = 0;
  for (auto& e: options) {
    uint value = 0;
    for (uint i = 0; i < e.type().outcomes().size(); i++) {
      auto v = e.GetNumOfModelsForOutcome(i);
      if (v > value) value = v;
    }
    if (min == 0 || value < min) {
      min = value;
      result = e.index();
    }
  }
  return result;
}

uint Strategy::breaker_exp(vec<Option>& options, Game&) {
  int result;
  double min = -1;
  for (auto& e: options) {
    int sumsq = 0;
    for (uint i = 0; i < e.type().outcomes().size(); i++) {
      auto v = e.GetNumOfModelsForOutcome(i);
      sumsq += v*v;
    }
    double value = (double)sumsq/e.GetTotalNumOfModels();
    if (min == -1 || value < min) {
      min = value;
      result = e.index();
    }
  }
  return result;
}

uint Strategy::breaker_entropy(vec<Option>& options, Game&) {
  int result;
  double max = 0;
  for (auto& e: options) {
    int total = e.GetTotalNumOfModels();
    double value = 0;
    for (uint i = 0; i < e.type().outcomes().size(); i++) {
      double p = (double)e.GetNumOfModelsForOutcome(i)/total;
      value -= p * log2(p);
    }
    if (value > max) {
      max = value;
      result = e.index();
    }
  }
  return result;
}

uint Strategy::breaker_parts(vec<Option>& options, Game&) {
  int result;
  double max = 0;
  for (auto& e: options) {
    auto value = e.GetNumOfSatOutcomes();
    if (value > max) {
      max = value;
      result = e.index();
    }
  }
  return result;
}

uint Strategy::breaker_fixed(vec<Option>& options, Game&) {
  int result = -1;
  double max = 0;
  for (auto& e: options) {
    int value = -1;
    for (uint i = 0; i < e.type().outcomes().size(); i++) {
      int f = e.GetNumOfFixedVarsForOutcome(i);
      if (value == -1 || f < value)
        value = f;
    }
    if (value > max) {
      max = value;
      result = e.index();
    }
  }
  assert(result != -1);
  return result;
}

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

namespace strategy {

namespace {
  uint maximize(std::function<double(Option&)> f, vec<Option>& options) {
    vec<double> values(options.size(), 0);
    std::transform(options.begin(), options.end(), values.begin(), f);
    auto min = std::max_element(values.begin(), values.end());
    return std::distance(values.begin(), min);
  }

  uint minimize(std::function<double(Option&)> f, vec<Option>& options) {
    vec<double> values(options.size(), 0);
    std::transform(options.begin(), options.end(), values.begin(), f);
    auto min = std::min_element(values.begin(), values.end());
    return std::distance(values.begin(), min);
  }
}

uint breaker::interactive(vec<Option>& options) {
  // Print options.
  printf("Select an experiment: \n");//%i: \n", exp_num++);
  for (auto& experiment: options) {
    if (experiment.GetNumOfSatOutcomes() > 1) {
      printf("%i) %s [ %s ] ",
        experiment.index(),
        experiment.type().name().c_str(),
        experiment.type().game().ParamsToStr(experiment.params()).c_str());
      printf(" ] - M: ");
      for (uint i = 0; i < experiment.type().outcomes().size(); i++)
        printf("%i ", experiment.GetNumOfModels()[i]);
      printf("F: ");
      for (uint i = 0; i < experiment.type().outcomes().size(); i++)
        printf("%i ", experiment.GetNumOfFixedVars()[i]);
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

uint maker::interactive(Option& option) {
  // Print options.
  printf("Select an outcome: \n");
  auto& type = option.type();
  for (uint i = 0; i < type.outcomes().size(); i++) {
    if (option.IsOutcomeSat(i)) printf("%i) ", i);
    else printf("-) ");
    printf("%s %s\n",
      type.outcomes_names()[i].c_str(),
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

uint breaker::random(vec<Option>& options) {
  return rand() % options.size();
}

uint maker::random(Option& option) {
  // Print options.
  int p = rand() % option.GetNumOfSatOutcomes();
  int i = -1;
  while (p >= 0) {
    i++;
    if (option.IsOutcomeSat(i)) p--;
  }
  return i;
}

uint maker::max_num(Option& option) {
  auto models = option.GetNumOfModels();
  auto max = std::max_element(models.begin(), models.end());
  return std::distance(models.begin(), max);
}

uint maker::fixed(Option& option) {
  auto fixed = option.GetNumOfFixedVars();
  auto min = std::min_element(fixed.begin(), fixed.end());
  return std::distance(fixed.begin(), min);
}

uint breaker::min_num(vec<Option>& options) {
  return minimize([](Option& o){
    auto models = o.GetNumOfModels();
    return *(std::max_element(models.begin(), models.end()));
  }, options);
}

uint breaker::exp_num(vec<Option>& options) {
  return minimize([](Option& o){
    auto models = o.GetNumOfModels();
    int sumsq = 0;
    for (uint i = 0; i < o.type().outcomes().size(); i++) {
      auto v = models[i];
      sumsq += v*v;
    }
    return (double)sumsq/o.GetTotalNumOfModels();
  }, options);
}

uint breaker::entropy(vec<Option>& options) {
  return maximize([](Option& o){
    auto models = o.GetNumOfModels();
    int total = o.GetTotalNumOfModels();
    double value = 0;
    for (uint i = 0; i < o.type().outcomes().size(); i++) {
      double p = (double) models[i]/total;
      value -= p * log2(p);
    }
    return value;
  }, options);
}

uint breaker::parts(vec<Option>& options) {
  return maximize(&Option::GetNumOfSatOutcomes, options);
}

uint breaker::fixed(vec<Option>& options) {
  return maximize([](Option& o){
    auto fixed = o.GetNumOfFixedVars();
    return *std::min_element(fixed.begin(), fixed.end());
  }, options);
}

}

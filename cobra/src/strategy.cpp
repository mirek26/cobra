/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include "formula.h"
#include "game.h"
#include "experiment.h"
#include "common.h"
#include "parser.h"

#include "strategy.h"

uint Strategy::breaker_interactive(vec<ExperimentSpec>& options, Game& game) {
  // Print options.
  printf("Select experiment: \n");//%i: \n", exp_num++);
  int o = 0;
  for (auto& e: options) {
    printf("%i) %s [ ", o++, e.type->name().c_str());
    for (auto k: e.params)
      printf("%s ", game.alphabet()[k].c_str());
    printf("] (%i sat outcomes)\n", e.sat_outcomes_num);
  }

  // User prompt.
  uint eid;
  bool ok;
  string str;
  do {
    printf("> ");
    ok = readIntOrString(eid, str);
  } while (!ok || eid >= options.size());
  return eid;
}

uint Strategy::maker_interactive(ExperimentSpec& option, Game&) {
  // Print options.
  printf("Select outcome: \n");
  auto e = option.type;
  auto params = option.params;
  for (uint i = 0; i < e->outcomes().size(); i++) {
    if (option.sat_outcomes[i]) printf("%i) ", i);
    else printf("-) ");
    printf("%s - %s %s\n",
      e->outcomes_names()[i].c_str(),
      e->outcomes()[i]->pretty(true, &params).c_str(),
      option.sat_outcomes[i] ? "" : "(unsatisfiable)");
  }

  // User prompt.
  uint oid;
  bool ok;
  string str;
  do {
    printf("> ");
    ok = readIntOrString(oid, str);
  } while (!ok || oid >= e->outcomes().size() ||
           !option.sat_outcomes[oid]);
  return oid;
}


uint Strategy::breaker_random(vec<ExperimentSpec>& options, Game&) {
  return rand() % options.size();
}

uint Strategy::maker_random(ExperimentSpec& option, Game&) {
  // Print options.
  int p = rand() % option.sat_outcomes_num;
  int i = -1;
  while (p >= 0) {
    i++;
    if (option.sat_outcomes[i]) p--;
  }
  return i;
}
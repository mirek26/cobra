/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <set>
#include <iostream>

#include "formula.h"
#include "game.h"
#include "experiment.h"
#include "common.h"
#include "parser.h"

extern "C" int yyparse();
extern "C" FILE* yyin;
extern Parser m;

struct ExperimentSpec {
  Experiment* type;
  std::vector<Variable*> params;
  std::vector<int> f_outcomes;
};

int main(int argc, char* argv[]) {
  // PARSE INPUT
  assert(argc == 2); // TODO: find some library to parse cmd line args
  auto file = argv[1];
  if (!(yyin = fopen(file, "r"))) {
    printf("Cannot open %s: %s.", file, strerror(errno));
    exit(EXIT_FAILURE);
  }
  try {
    yyparse();
  } catch (ParserException* p) {
    printf("Invalid input: %s\n", p->what());
    exit(1);
  }
  fclose (yyin);
  printf("Game loaded.\n");

  // INTERACTIVE MODE
  Game& g = m.game();
  g.Precompute();

  printf("Starting interactive mode [both].\n");
  CnfFormula current;
  current.AddConstraint(g.restriction());

  int exp_num = 1;
  while (true) {
    auto varEquiv = current.ComputeVariableEquivalence(g.variables().size());
    int o = 0;
    std::vector<std::pair<Experiment*, std::vector<CharId>>> experiments;
    printf("Select experiment #%i: \n", exp_num++);
    for (auto e: g.experiments()) {
      auto params_all = e->GenerateParametrizations(varEquiv);
      for (auto& params: *params_all) {
        experiments.push_back(std::make_pair(e, params));
        // print option
        printf("%i) %s [ ", o++, e->name().c_str());
        for (auto k: params) printf("%s ", g.alphabet()[k].c_str());
        printf("]\n");
        // analysis
        // printf("[fixed:");
        // for (auto& outcome: e->outcomes()) {
        //   CnfFormula n;
        //   n.AddConstraint(current);
        //   CnfFormula nn = outcome.SubstituteParams(p);
        //   n.AddConstraint(nn);
        //   if (n.Satisfiable()) {
        //     auto fixed = n.GetFixedVariables();
        //     experiments.back().f_outcomes.push_back(fixed);
        //     printf(" %i", fixed);
        //   } else {
        //     experiments.back().f_outcomes.push_back(-1);
        //   }
        // }
        //printf("]\n");
      }
    }

    int eid, oid;
    bool ok;
    std::string str;
    do {
      printf("> ");
      ok = readIntOrString(eid, str);
    } while (!ok || eid >= experiments.size());

    printf("Select outcome: \n");
    o = 0;
    auto experiment = experiments[eid].first;
    auto param = experiments[eid].second;
    for (uint i = 0; i < experiment->outcomes().size(); i++) {
      printf("%i) %s - %s\n", i, experiment->outcomes_names()[i].c_str(),
                  experiment->outcomes()[i]->pretty(true, &param).c_str());
    }
    do {
      printf("> ");
      ok = readIntOrString(oid, str);
    } while (!ok || oid >= experiment->outcomes().size());

    printf("Gained knowledge: %s\n", experiment->outcomes()[oid]->pretty(true, &param).c_str());
    auto newConstraint = experiment->outcomes()[oid]->ToCnf(param);
    current.AddConstraint(*newConstraint);
    delete newConstraint;
    if (current.HasOnlyOneModel()) {
      printf("\nSOLVED!\n");
      current.Satisfiable();
      current.PrintAssignment(g.variables().size());
      break;
    }
  }

  return 0;
}
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
  std::vector<CharId> params;
  std::vector<bool> sat_outcomes;
};

int main(int argc, char* argv[]) {
  // PARSE INPUT
  assert(argc == 2);
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
    std::vector<ExperimentSpec> experiments;
    printf("Select experiment #%i: \n", exp_num++);
    for (auto e: g.experiments()) {
      auto params_all = e->GenerateParametrizations(varEquiv);
      for (auto& params: *params_all) {
        experiments.push_back({ e, params, std::vector<bool>(e->outcomes().size(), false)});
        // print option
        printf("%i) %s [ ", o++, e->name().c_str());
        for (auto k: params) printf("%s ", g.alphabet()[k].c_str());
        printf("]\n");
        // basic analysis
        for (uint i = 0; i < e->outcomes().size(); i++) {
          CnfFormula n;
          n.AddConstraint(current);
          n.AddConstraint(e->outcomes()[i], params);
          n.InitSolver();
          if (n.Satisfiable()) {
            //auto fixed = n.GetFixedVariables();
            experiments.back().sat_outcomes[i] = true;
            //printf(" %i", fixed);
          }
        }
      }
    }

    uint eid, oid;
    bool ok;
    std::string str;
    do {
      printf("> ");
      ok = readIntOrString(eid, str);
    } while (!ok || eid >= experiments.size());

    printf("Select outcome: \n");
    o = 0;
    auto experiment = experiments[eid].type;
    auto params = experiments[eid].params;
    for (uint i = 0; i < experiment->outcomes().size(); i++) {
      if (experiments[eid].sat_outcomes[i]) printf("%i) ", i);
      else printf("-) ");
      printf("%s - %s %s\n",
        experiment->outcomes_names()[i].c_str(),
        experiment->outcomes()[i]->pretty(true, &params).c_str(),
        experiments[eid].sat_outcomes[i] ? "" : "(unsatisfiable)");
    }
    do {
      printf("> ");
      ok = readIntOrString(oid, str);
    } while (!ok || oid >= experiment->outcomes().size() ||
             !experiments[eid].sat_outcomes[oid]);

    printf("Gained knowledge: %s\n", experiment->outcomes()[oid]->pretty(true, &params).c_str());
    auto newConstraint = experiment->outcomes()[oid]->ToCnf(params);
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
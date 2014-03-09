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
  yyparse();
  fclose (yyin);
  printf("LOADED.\n");

  // // INTERACTIVE MODE
  Game& g = m.game();
  //if (!g.complete()) exit(1);
  g.Precompute();
  printf("PRECOMPUTED.\n");

  std::vector<int> groups = { 1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  //std::string s = "x1A";
  //groups[m.get<Variable>(s)->id()] = 2;
  for (auto e: g.experiments()) {
    printf("\n\n%s\n\n", e->name().c_str());
    e->GenerateParametrizations(groups);
  }

  printf("DONE.\n");
  // CnfFormula current;
  // current.AddConstraint(g.init());

  // while (true) {
  //   auto varEquiv = current.ComputeVariableEquivalence();
  //   int o = 0;
  //   std::vector<ExperimentSpec> experiments;
  //   for (auto e: g.experiments()) {
  //     auto parametrizations = e->param()->GenerateAll(varEquiv);
  //     for (auto& p: parametrizations) {
  //       experiments.push_back({ e, p, {} });
  //       // print option
  //       printf("%i) %s ", o++, e->experiment_name().c_str());
  //       for (auto v: p) printf("%s ", v->pretty().c_str());
  //       // analysis
  //       printf("[fixed:");
  //       for (auto& outcome: e->outcomes()) {
  //         CnfFormula n;
  //         n.AddConstraint(current);
  //         CnfFormula nn = outcome.SubstituteParams(p);
  //         n.AddConstraint(nn);
  //         if (n.Satisfiable()) {
  //           auto fixed = n.GetFixedVariables();
  //           experiments.back().f_outcomes.push_back(fixed);
  //           printf(" %i", fixed);
  //         } else {
  //           experiments.back().f_outcomes.push_back(-1);
  //         }
  //       }
  //       printf("]\n");
  //     }
  //   }
  //   uint ch, outcome;
  //   do {
  //     printf("Your choice: ");
  //     std::cin >> ch;
  //     std::cin.clear();
  //     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  //     //scanf("%i", &ch);
  //   } while (std::cin.fail() || ch >= experiments.size());
  //   do {
  //     printf("Select outcome[");
  //     for (uint i = 0; i < experiments[ch].f_outcomes.size(); i++)
  //       if (experiments[ch].f_outcomes[i] >= 0) printf("%i", i);
  //     printf("]: ");
  //     std::cin >> outcome;
  //     std::cin.clear();
  //     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  //   } while (outcome >= experiments[ch].f_outcomes.size() ||
  //            experiments[ch].f_outcomes[outcome] == -1);
  //   auto newConstraint = experiments[ch].type->outcomes()[outcome].SubstituteParams(experiments[ch].params);
  //   current.AddConstraint(newConstraint);
  //   if (current.HasOnlyOneModel()) {
  //     printf("\nSOLVED!\n");
  //     current.Satisfiable();
  //     current.PrintAssignment();
  //     break;
  //   }
  // }

  return 0;
}
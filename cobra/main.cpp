/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cerrno>
#include <set>

#include "formula.h"
#include "util.h"
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

  // INTERACTIVE MODE
  Game& g = m.game();
  if (!g.complete()) exit(1);

  CnfFormula current;
  current.AddConstraint(g.init());

  while (true) {
    auto varEquiv = current.ComputeVariableEquivalence();
    int o = 0;
    std::vector<ExperimentSpec> experiments;
    for (auto e: g.experiments()) {
      auto parametrizations = e->param()->GenerateAll(varEquiv);
      for (auto& p: parametrizations) {
        experiments.push_back({ e, p, {} });
        // print option
        printf("%i) %s ", o++, e->experiment_name().c_str());
        for (auto v: p) printf("%s ", v->pretty().c_str());
        // analysis
        printf("[fixed:");
        for (auto& outcome: e->outcomes()) {
          CnfFormula n;
          n.AddConstraint(current);
          CnfFormula nn = outcome.SubstituteParams(p);
          n.AddConstraint(nn);
          if (n.Satisfiable()) {
            auto fixed = n.GetFixedVariables();
            experiments.back().f_outcomes.push_back(fixed);
            printf(" %i", fixed);
          } else {
            experiments.back().f_outcomes.push_back(-1);
          }
        }
        printf("]\n");
      }
    }
    uint ch, outcome;
    do {
      printf("Your choice: ");
      scanf("%i", &ch);
    } while (ch >= experiments.size());
    do {
      printf("Select outcome[");
      for (uint i = 0; i < experiments[ch].f_outcomes.size(); i++)
        if (experiments[ch].f_outcomes[i] >= 0) printf("%i", i);
      printf("]: ");
      scanf("%i", &outcome);
    } while (outcome >= experiments[ch].f_outcomes.size() ||
             experiments[ch].f_outcomes[outcome] == -1);
    auto newConstraint = experiments[ch].type->outcomes()[outcome].SubstituteParams(experiments[ch].params);
    current.AddConstraint(newConstraint);
  }

  return 0;
}
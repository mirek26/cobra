/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <set>
#include <iostream>
#include <bliss/graph.hh>
#include <bliss/utils.hh>

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

void print_usage() {
  printf("Usage: cobra-backend filename\n");
}

void parse_input(int argc, char* argv[]) {
  // PARSE INPUT
  if (argc != 2) {
    print_usage();
    exit(EXIT_FAILURE);
  }
  auto file = argv[1];
  if (!(yyin = fopen(file, "r"))) {
    printf("Cannot open %s: %s.", file, strerror(errno));
    exit(EXIT_FAILURE);
  }
  try {
    yyparse();
  } catch (ParserException* p) {
    printf("Invalid input: %s\n", p->what());
    exit(EXIT_FAILURE);
  }
  fclose (yyin);
  printf("Game loaded.\n");
}

int main(int argc, char* argv[]) {
  parse_input(argc, argv);

  // INTERACTIVE MODE
  Game& game = m.game();
  game.Precompute();

  printf("Starting interactive mode [both].\n");
  auto knowledge_graph = game.CreateGraph();
  game.restriction()->AddToGraph(*knowledge_graph, nullptr);

  CnfFormula knowledge;
  knowledge.AddConstraint(game.restriction());

  int exp_num = 1;
  while (true) {
    // compute var equivalence
    auto var_equiv = game.ComputeVarEquiv(*knowledge_graph);
    //
    int o = 0;
    std::vector<ExperimentSpec> experiments;
    for (auto e: game.experiments()) {
      auto params_all = e->GenerateParametrizations(var_equiv);
      for (auto& params: *params_all) {
        experiments.push_back(
          { e,
            params,
            std::vector<bool>(e->outcomes().size(), false)});
        // basic analysis
        int sat_outcomes = 0;
        for (uint i = 0; i < e->outcomes().size(); i++) {
          CnfFormula n;
          n.AddConstraint(knowledge);
          n.AddConstraint(e->outcomes()[i], params);
          n.InitSolver();
          if (n.Satisfiable()) {
            //auto fixed = n.GetFixedVariables();
            experiments.back().sat_outcomes[i] = true;
            sat_outcomes++;
            //printf(" %i", fixed);
          }
        }
        assert(sat_outcomes > 0);
        if (sat_outcomes == 1) {
          experiments.pop_back();
          continue;
        }
        // print option
        if (experiments.size() == 1) {
          printf("Select experiment #%i: \n", exp_num++);
        }
        printf("%i) %s [ ", o++, e->name().c_str());
        for (auto k: params)
          printf("%s ", game.alphabet()[k].c_str());
        printf("] (%i sat outcomes)\n", sat_outcomes);
      }
    }

    if (experiments.size() == 0) {
      printf("SOLVED! (You cannot get any new information from the experiments.)\n");
      knowledge.InitSolver();
      knowledge.Satisfiable();
      knowledge.PrintAssignment(game.variables());
      break;
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

    auto outcome = experiment->outcomes()[oid];
    printf("Gained knowledge: %s\n", outcome->pretty(true, &params).c_str());
    knowledge.AddConstraint(outcome, params);
    knowledge.InitSolver();
    knowledge.WriteDimacs(stdout);
    outcome->AddToGraph(*knowledge_graph, &params);
  }

  return 0;
}
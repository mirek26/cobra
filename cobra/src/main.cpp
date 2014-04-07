/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <set>
#include <iostream>
#include <cmath>
#include <ctime>
#include <bliss/graph.hh>
#include <bliss/utils.hh>
#include <tclap/CmdLine.h>

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
  vec<CharId> params;
  vec<bool> sat_outcomes;
};

void print_stats(Game& game) {
  printf("===== GAME STATISTICS =====\n");
  printf("Num of variables: %lu\n", game.variables().size());
  CnfFormula knowledge;
  knowledge.AddConstraint(game.restriction());
  uint models = knowledge.NumberOfModelsSat(game.variables().size());
  printf("Num of possible codes: %u\n\n", models);

  uint branching = 0, total = 0;
  for (auto e: game.experiments()) {
    if (e->outcomes().size() > branching) {
      branching = e->outcomes().size();
    }
    total += e->NumberOfParametrizations();
  }

  printf("Num of types of experiments: %lu\n", game.experiments().size());
  printf("Alphabet size: %lu\n", game.alphabet().size());
  printf("Total num of experiments: %u\n", total);
  printf("Avg num of parametrizations per type: %.2f\n",
         (float)total/game.experiments().size());
  printf("Maximal branching: %u\n", branching);
  double d = log(models)/log(branching);
  printf("Trivial lower bound (expected): %.2f\n", d);
  printf("Trivial lower bound (worst-case): %.0f\n", ceil(d));
  printf("===========================\n");
}

void play_mode() {
  // INTERACTIVE MODE
  Game& game = m.game();
  game.Precompute();
  printf("Starting interactive mode [both].\n");
  auto knowledge_graph = game.CreateGraph();
  game.restriction()->AddToGraph(*knowledge_graph, nullptr);

  CnfFormula knowledge;
  knowledge.AddConstraint(game.restriction());
  knowledge.InitSolver();
  knowledge.WriteDimacs(stdout);

  int exp_num = 1;
  while (true) {
    // Compute var equivalence.
    auto var_equiv = game.ComputeVarEquiv(*knowledge_graph);
    //
    int o = 0;
    vec<ExperimentSpec> experiments;
    for (auto e: game.experiments()) {
      auto params_all = e->GenParams(var_equiv);
      for (auto& params: *params_all) {
        experiments.push_back(
          { e,
            params,
            vec<bool>(e->outcomes().size(), false)});
        // Perform basic analysis.
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
        // Print option.
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
    string str;
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
}


int main(int argc, char* argv[]) {
  // Parse input with TCLAP library.
  try {
    using namespace TCLAP;
    CmdLine cmd("Code Breaking Game Analyzer blah blah blah", ' ', "0.1");
    SwitchArg infoArg(
      "i", "info",
      "Print basic information about the game.",
      cmd, false);
    SwitchArg playArg(
      "p", "play",
      "Starts interactive mode, player strategies can be specified by -b, -m.",
      cmd, false);

    vec<string> greedy_stgs({ "max", "exp", "entropy",
      "parts", "random", "interactive" });
    ValuesConstraint<string> greedyConst(greedy_stgs);
    ValueArg<string> codemakerArg(
      "m", "codemaker",
      "Strategy to be played by the codemaker. Default: interactive.", false,
      "interactive", &greedyConst);
    ValueArg<string> codebreakerArg(
      "b", "codebreaker",
      "Strategy to be played by the codebreaker. Default: interactive.", false,
      "interactive", &greedyConst);
    cmd.add(codemakerArg);
    cmd.add(codebreakerArg);

    UnlabeledValueArg<std::string> filenameArg(
      "filename",
      "Input file name.", false,
      "", "file name");
    cmd.add(filenameArg);
    cmd.parse(argc, argv);

    auto file = filenameArg.getValue();
    if (!(yyin = fopen(file.c_str(), "r"))) {
      printf("Cannot open %s: %s.\n", file.c_str(), strerror(errno));
      exit(EXIT_FAILURE);
    }
    auto t1 = clock();
    try {
      yyparse();
    } catch (ParserException* p) {
      printf("Invalid input: %s\n", p->what());
      exit(EXIT_FAILURE);
    }
    fclose (yyin);
    auto t2 = clock();
    printf("Input loaded in %.2fs.\n", double(t2 - t1)/CLOCKS_PER_SEC);

    if (infoArg.getValue()) {
      print_stats(m.game());
    }

    if (playArg.getValue()) {
      play_mode();
    }

  } catch (TCLAP::ArgException &e) {
    std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
  }

  return 0;
}
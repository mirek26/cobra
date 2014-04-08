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
#include "strategy.h"

extern "C" int yyparse();
extern "C" FILE* yyin;
extern Parser m;

std::function<uint(vec<Option>&, Game& game)> g_breakerStg;
std::function<uint(Option&, Game& game)> g_makerStg;

void print_stats(Game& game) {
  printf("\n===== GAME STATISTICS =====\n");
  printf("Num of variables: %lu\n", game.variables().size());
  CnfFormula knowledge(game.variables().size());
  knowledge.AddConstraint(game.restriction());
  uint models = knowledge.NumOfModels();
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

void simulation_mode() {
  Game& game = m.game();
  game.Precompute();
  printf("\n===== SIMULATION MODE =====\n\n");
  auto knowledge_graph = game.CreateGraph();
  game.restriction()->AddToGraph(*knowledge_graph, nullptr);

  CnfFormula knowledge(game.variables().size());
  knowledge.AddConstraint(game.restriction());
  //knowledge.WriteDimacs(stdout);

  int exp_num = 1;
  while (true) {
    // Compute var equivalence.
    auto var_equiv = game.ComputeVarEquiv(*knowledge_graph);
    int t = game.variables().size(), f = t + 1;
    for (uint i = 1; i <= game.variables().size(); i++) {
      if (knowledge.MustBeTrue(i)) var_equiv[i] = t;
      else if (knowledge.MustBeFalse(i)) var_equiv[i] = f;
    }
    // printf("Var equiv:\n");
    // for (auto i: var_equiv) {
    //   printf("%i ", i);
    // }
    // printf("\n");

    // Prepare list of all sensible experiments in this round.
    vec<Option> options;
    for (auto e: game.experiments()) {
      auto params_all = e->GenParams(var_equiv);
      for (auto& params: *params_all) {
        options.push_back(Option(knowledge, *e, params, options.size()));
      }
    }

    // Choose and print an experiment
    auto experiment = options[g_breakerStg(options, game)];
    printf("EXPERIMENT: %s ", experiment.type().name().c_str());
    for (auto k: experiment.params())
      printf("%s ", game.alphabet()[k].c_str());
    printf("\n");

    // Choose and print an outcome
    uint oid = g_makerStg(experiment, game);
    assert(oid < experiment.type().outcomes().size());
    assert(experiment.IsOutcomeSat(oid) == true);
    auto outcome = experiment.type().outcomes()[oid];
    printf("OUTCOME: %s\n", experiment.type().outcomes_names()[oid].c_str());
    printf("  ->     %s\n\n", outcome->pretty(true, &experiment.params()).c_str());

    // Check if solved.
    knowledge.AddConstraint(outcome, experiment.params());
    if (knowledge.NumOfModels() == 1) {
      printf("SOLVED in %i experiments!\n", exp_num);
      knowledge.Satisfiable();
      knowledge.PrintAssignment(game.variables());
      break;
    }

    // Prepare for another round.
    exp_num++;
    //knowledge.WriteDimacs(stdout);
    outcome->AddToGraph(*knowledge_graph, &experiment.params());
  }
}

int main(int argc, char* argv[]) {
  // Parse input with TCLAP library.
  srand(time(NULL));
  try {
    using namespace TCLAP;
    CmdLine cmd("Code Breaking Game Analyzer blah blah blah", ' ', "0.1");
    SwitchArg infoArg(
      "i", "info",
      "Print basic information about the game.",
      cmd, false);
    SwitchArg simArg(
      "s", "simulation",
      "Starts simulation mode, player strategies can be specified by -b, -m.",
      cmd, false);

    vec<string> greedy_stgs;
    for (auto x: Strategy::breaker_strategies)
      greedy_stgs.push_back(x.first);

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

    if (simArg.getValue()) {
      g_breakerStg = Strategy::breaker_strategies.at(codebreakerArg.getValue());
      g_makerStg = Strategy::maker_strategies.at(codemakerArg.getValue());
      simulation_mode();
    }

  } catch (TCLAP::ArgException &e) {
    std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
  }

  return 0;
}
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
#include <sys/stat.h>
#include "formula.h"
#include "game.h"
#include "experiment.h"
#include "common.h"
#include "parser.h"
#include "strategy.h"
#include "pico-solver.h"
#include "simple-solver.h"

extern "C" int yyparse();
extern "C" FILE* yyin;
extern Parser m;

std::function<uint(vec<Option>&)> g_breakerStg;
std::function<uint(Option&)> g_makerStg;

void print_head(string name) {
  printf("\n%s===== %s =====%s\n", color::shead, name.c_str(), color::snormal);
}

void time_overview(clock_t start) {
  print_head("TIME OVERVIEW");
  printf("Total time: %.2fs\n", toSeconds(clock() - start));
  auto s1 = PicoSolver::s_stats();
  printf("PicoSolver (calls/time): sat %i/%.2fs fixed %i/%.2fs models %i/%.2fs\n",
    s1.sat_calls, toSeconds(s1.sat_time),
    s1.fixed_calls, toSeconds(s1.fixed_time),
    s1.models_calls, toSeconds(s1.models_time));
  auto s2 = SimpleSolver::s_stats();
  printf("SimpleSolver (calls/time): sat %i/%.2fs fixed %i/%.2fs models %i/%.2fs\n",
    s2.sat_calls, toSeconds(s2.sat_time),
    s2.fixed_calls, toSeconds(s2.fixed_time),
    s2.models_calls, toSeconds(s2.models_time));

}

void print_stats(Game& game, string filename) {
  print_head("GAME OVERVIEW");
  printf("Num of variables: %lu\n", game.variables().size());
  PicoSolver s(game.variables(), game.restriction());
  Solver& solver = s;
  uint models = solver.NumOfModels();
  printf("Num of possible codes: %u\n\n", models);

  struct stat file_st;
  stat(filename.c_str(), &file_st);
  uint branching = 0, num_exp = 0, nodes = 0;
  for (auto e: game.experiments()) {
    for (auto f: e->outcomes()) {
      nodes += f->Size();
    }
    if (e->outcomes().size() > branching) {
      branching = e->outcomes().size();
    }
    num_exp += e->NumberOfParametrizations();
  }

  printf("Read from file: %s\n", filename.c_str());
  printf("File size: %lli\n", file_st.st_size);
  printf("Num of nodes in formulas: %u\n\n", nodes);


  printf("Num of types of experiments: %lu\n", game.experiments().size());
  printf("Alphabet size: %lu\n", game.alphabet().size());
  printf("Total num of experiments: %u\n", num_exp);
  printf("Avg num of parametrizations per type: %.2f\n",
         (float)num_exp/game.experiments().size());
  printf("Maximal branching: %u\n", branching);
  double d = log(models)/log(branching);
  printf("Trivial lower bound (expected): %.2f\n", d);
  printf("Trivial lower bound (worst-case): %.0f\n\n", ceil(d));

  printf("Well-formed check...");
  fflush(stdout);

  auto t1 = clock();
  game.Precompute();
  auto knowledge_graph = game.CreateGraph();
  game.restriction()->AddToGraph(*knowledge_graph, nullptr);
  auto var_equiv = game.ComputeVarEquiv(solver, *knowledge_graph);
  for (auto e: game.experiments()) {
    auto& params_all = e->GenParams(var_equiv);
    for (auto& params: params_all) {
      auto f1 = new vec<Formula*>(e->outcomes().begin(), e->outcomes().end());
      auto f2 = m.get<ExactlyOperator>(1, f1);
      auto f3 = m.get<ImpliesOperator>(game.restriction(), f2);
      auto f4 = m.get<NotOperator>(f3);

      solver.OpenContext();
      solver.AddConstraint(f4, params);
      if (solver.Satisfiable()) {
        printf("%s failed!%s\n", color::serror, color::snormal);
        printf("EXPERIMENT: %s %s", e->name().c_str(), game.ParamsToStr(params).c_str());
        printf("\nPROBLEMATIC ASSIGNMENT: \n");
        solver.PrintAssignment();
        printf("\n");
        return;
      }
    }
  }
  auto t2 = clock();
  printf("ok [%.2fs]\n", double(t2 - t1)/CLOCKS_PER_SEC);
}

void simulation_mode() {
  print_head("SIMULATION");
  Game& game = m.game();
  game.Precompute();
  auto knowledge_graph = game.CreateGraph();
  game.restriction()->AddToGraph(*knowledge_graph, nullptr);

  PicoSolver knowledge(game.variables(), game.restriction());
  //knowledge.WriteDimacs(stdout);

  int exp_num = 1;
  while (true) {
    auto options = game.GenerateExperiments(knowledge, *knowledge_graph);

    // Choose and print an experiment
    auto experiment = options[g_breakerStg(options)];
    printf("%sEXPERIMENT: %s %s %s\n",
           color::semph,
           experiment.type().name().c_str(),
           game.ParamsToStr(experiment.params()).c_str(),
           color::snormal);

    // Choose and print an outcome
    uint oid = g_makerStg(experiment);
    assert(oid < experiment.type().outcomes().size());
    assert(experiment.IsOutcomeSat(oid) == true);
    auto outcome = experiment.type().outcomes()[oid];
    printf("%sOUTCOME: %s\n",
           color::semph,
           experiment.type().outcomes_names()[oid].c_str());
    printf("  ->     %s\n\n%s",
           outcome->pretty(true, &experiment.params()).c_str(),
           color::snormal);

    // Check if solved.
    knowledge.AddConstraint(outcome, experiment.params());
    if (knowledge.NumOfModels() == 1) { // TODO: nepotrebuju presne, staci vedet jestli jich neni vic
      printf("%sSOLVED in %i experiments!%s\n", color::shead, exp_num, color::snormal);
      knowledge.Satisfiable();
      knowledge.PrintAssignment();
      break;
    }

    // Prepare for another round.
    exp_num++;
    //knowledge.WriteDimacs(stdout);
    outcome->AddToGraph(*knowledge_graph, &experiment.params());
  }
  delete knowledge_graph;
}

void analyze(PicoSolver& solver, bliss::Digraph& graph, uint depth, uint& max, uint& sum) {
  Game& game = m.game();
  auto options = game.GenerateExperiments(solver, graph);
  auto experiment = options[g_breakerStg(options)];
  for (uint i = 0; i < experiment.type().outcomes().size(); i++) {
    solver.OpenContext();
    auto outcome = experiment.type().outcomes()[i];
    solver.AddConstraint(outcome, experiment.params());
    bool sat = solver.Satisfiable(), one = false;
    if (sat) one = solver.OnlyOneModel();
    if (one) {
      sum += depth;
      max = std::max(max, depth);
    } else if (sat) {
      bliss::Digraph ngraph(graph);
      outcome->AddToGraph(ngraph, &experiment.params());
      analyze(solver, ngraph, depth + 1, max, sum);
    }
    solver.CloseContext();
  }
}

void analyze_mode() {
  print_head("STRATEGY ANALYSIS");
  Game& game = m.game();
  game.Precompute();
  auto graph = game.CreateGraph();
  game.restriction()->AddToGraph(*graph, nullptr);
  PicoSolver solver(game.variables(), game.restriction());
  uint max = 0, sum = 0;
  analyze(solver, *graph, 1, max, sum);
  printf("Worst-case: %u\n", max);
  printf("Average-case: %.2f (%u)\n", (double)sum/solver.NumOfModels(), sum);
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
    SwitchArg analyzeArg(
      "a", "analyze",
      "Analyze codebreaker's strategy.",
      cmd, false);

    vec<string> breaker_stgs, maker_stgs;
    string breaker_man = "", maker_man = "";
    for (auto s: strategy::breaker_strategies) {
      breaker_stgs.push_back(s.first);
      breaker_man += "\n" + color::emph + s.first + ": " + color::normal + s.second.first;
    }
    for (auto s: strategy::maker_strategies) {
      maker_stgs.push_back(s.first);
      maker_man += "\n" + color::emph + s.first + ": " + color::normal + s.second.first;
    }

    ValuesConstraint<string> makerConstraint(maker_stgs);
    ValueArg<string> codemakerArg(
      "1", "codemaker",
      "Strategy to be played by the codemaker. Default: interactive." + maker_man, false,
      "interactive", &makerConstraint);
    ValuesConstraint<string> breakerConstraint(breaker_stgs);
    ValueArg<string> codebreakerArg(
      "2", "codebreaker",
      "Strategy to be played by the codebreaker. Default: interactive." + breaker_man, false,
      "interactive", &breakerConstraint);
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
    printf("Loading... ");
    try {
      yyparse();
    } catch (ParserException* p) {
      printf("\nInvalid input: %s\n", p->what());
      exit(EXIT_FAILURE);
    }
    fclose (yyin);
    auto t2 = clock();
    printf("[%.2fs]\n", double(t2 - t1)/CLOCKS_PER_SEC);
    t1 = clock();

    if (infoArg.getValue() || (!simArg.getValue() && !analyzeArg.getValue())) {
      print_stats(m.game(), file);
    }

    if (simArg.getValue()) {
      g_breakerStg = strategy::breaker_strategies.at(codebreakerArg.getValue()).second;
      g_makerStg = strategy::maker_strategies.at(codemakerArg.getValue()).second;
      simulation_mode();
    }

    if (analyzeArg.getValue()) {
      if (codebreakerArg.getValue() == "interactive") {
        printf("Cannot analyze strategy 'interactive'. \n");
        exit(EXIT_FAILURE);
      }
      g_breakerStg = strategy::breaker_strategies.at(codebreakerArg.getValue()).second;
      analyze_mode();
    }

    time_overview(t1);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
  }

  return 0;
}
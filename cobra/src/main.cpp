/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <set>
#include <string>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <bliss/graph.hh>
#include <bliss/utils.hh>
#include <tclap/CmdLine.h>
#include "./formula.h"
#include "./game.h"
#include "./experiment.h"
#include "./common.h"
#include "./parser.h"
#include "./strategy.h"
#include "./minisolver.h"
#include "./picosolver.h"
#include "./simple-solver.h"

extern "C" int yyparse();
extern "C" FILE* yyin;
extern Parser m;

std::function<uint(vec<Experiment>&)> g_breakerStg;
std::function<uint(Experiment&)> g_makerStg;

typedef struct Args {
  string filename;
  string mode;
  string backend;
  string stg_experiment;
  string stg_outcome;
} Args;

Args args;

Solver* get_solver(uint var_count, Formula* constraint = nullptr) {
  if (args.backend == "picosat") {
    return new PicoSolver(var_count, constraint);
  } else if (args.backend == "minisat") {
    return new MiniSolver(var_count, constraint);
  } else if (args.backend == "simple") {
    return new SimpleSolver(var_count, constraint);
  }
  assert(false);
}

void print_head(string name) {
  printf("\n%s===== %s =====%s\n", color::shead, name.c_str(), color::snormal);
}

void time_overview(clock_t start) {
  print_head("TIME OVERVIEW");
  printf("Total time: %.2fs\n", toSeconds(clock() - start));
  printf("Bliss (calls/time): %u/%.2fs\n",
         Game::bliss_calls, toSeconds(Game::bliss_time));
  auto s1 = PicoSolver::s_stats();
  printf(
    "PicoSolver (calls/time): sat %i/%.2fs fixed %i/%.2fs models %i/%.2fs\n",
    s1.sat_calls, toSeconds(s1.sat_time),
    s1.fixed_calls, toSeconds(s1.fixed_time),
    s1.models_calls, toSeconds(s1.models_time));
  auto s2 = MiniSolver::s_stats();
  printf(
    "MiniSolver (calls/time): sat %i/%.2fs fixed %i/%.2fs models %i/%.2fs\n",
    s2.sat_calls, toSeconds(s2.sat_time),
    s2.fixed_calls, toSeconds(s2.fixed_time),
    s2.models_calls, toSeconds(s2.models_time));
  auto s3 = SimpleSolver::s_stats();
  printf(
    "SimpleSolver (calls/time): sat %i/%.2fs fixed %i/%.2fs models %i/%.2fs\n",
    s3.sat_calls, toSeconds(s3.sat_time),
    s3.fixed_calls, toSeconds(s3.fixed_time),
    s3.models_calls, toSeconds(s3.models_time));
}

void overview_mode() {
  print_head("GAME OVERVIEW");
  Game& game = m.game();

  printf("Num of variables: %lu\n", game.vars().size());
  Solver* solver = get_solver(game.vars().size(), game.constraint());
  uint models = solver->NumOfModels();
  printf("Num of possible codes: %u\n\n", models);

  struct stat file_st;
  stat(args.filename.c_str(), &file_st);
  uint branching = 0, num_exp = 0, nodes = 0;
  for (auto e : game.experiments()) {
    for (auto f : e->outcomes()) {
      nodes += f.formula->Size();
    }
    if (e->outcomes().size() > branching) {
      branching = e->outcomes().size();
    }
    num_exp += e->NumberOfParametrizations();
  }

  printf("Read from file: %s\n", args.filename.c_str());
  printf("File size: %lli\n", file_st.st_size);
  printf("Num of nodes in formulas: %u\n\n", nodes);


  printf("Num of types of experiments: %lu\n", game.experiments().size());
  printf("Alphabet size: %lu\n", game.alphabet().size());
  printf("Total num of experiments: %u\n", num_exp);
  printf("Avg num of parametrizations per type: %.2f\n",
         static_cast<float>(num_exp) / game.experiments().size());
  printf("Maximal branching: %u\n", branching);
  double d = log(models)/log(branching);
  printf("Trivial lower bound (expected): %.2f\n", d);
  printf("Trivial lower bound (worst-case): %.0f\n\n", ceil(d));

  printf("Well-formed check...");
  fflush(stdout);

  auto t1 = clock();

  ExpGenerator gen(game, *solver, vec<EvalExp>());
  for (auto& e : gen.All()) {
    auto f1 = new vec<Formula*>();
    for (auto o : e.type().outcomes()) f1->push_back(o.formula);
    auto f2 = m.get<ExactlyOperator>(1, f1);
    auto f3 = m.get<NotOperator>(f2);

    solver->OpenContext();
    solver->AddConstraint(f3, e.params());
    if (solver->Satisfiable()) {
      printf("%s failed!%s\n", color::serror, color::snormal);
      printf("EXPERIMENT: %s %s",
             e.type().name().c_str(), game.ParamsToStr(e.params()).c_str());
      printf("\nPROBLEMATIC ASSIGNMENT: \n");
      game.PrintModel(solver->GetModel());
      printf("\n");
      return;
    }
    solver->CloseContext();
  }

  delete solver;
  auto t2 = clock();
  printf("ok [%.2fs]\n", static_cast<double>(t2 - t1)/CLOCKS_PER_SEC);
}

void simulation_mode() {
  print_head("SIMULATION");
  Game& game = m.game();
  Solver* solver = get_solver(game.vars().size(), game.constraint());

  int exp_num = 1;
  vec<EvalExp> process;
  while (true) {
    ExpGenerator gen(game, *solver, process);
    auto options = gen.All();

    // Choose and print an experiment
    auto& experiment = options[g_breakerStg(options)];
    printf("%sEXPERIMENT: %s %s %s\n",
           color::semph,
           experiment.type().name().c_str(),
           game.ParamsToStr(experiment.params()).c_str(),
           color::snormal);
    if (process.size() > 0 &&
        &experiment.type() == &process.back().exp.type() &&
        experiment.params() == process.back().exp.params()) {
      printf("%sNOT SOLVED! (%i experiments)%s\n",
             color::shead, exp_num, color::snormal);
      break;
    }

    // Choose and print an outcome
    uint oid = g_makerStg(experiment);
    assert(oid < experiment.type().outcomes().size());
    assert(experiment.IsSat(oid) == true);
    auto outcome = experiment.type().outcomes()[oid];
    printf("%sOUTCOME: %s\n",
           color::semph,
           outcome.name.c_str());
    auto knowledge = outcome.formula->pretty(true, &experiment.params());
    if (knowledge.length() > 200) knowledge = knowledge.substr(0, 200) + "...";
    printf("  ->   %s\n\n%s",
           knowledge.c_str(),
           color::snormal);

    // Check if solved.
    solver->AddConstraint(outcome.formula, experiment.params());
    auto sat = solver->Satisfiable();
    assert(sat);
    auto model = solver->GetModel();
    int final = experiment.type().final_outcome();
    if (solver->OnlyOneModel() && (final == -1 || final == static_cast<int>(oid))) {
      printf("%sSOLVED in %i experiments!%s\n",
             color::shead, exp_num, color::snormal);
      game.PrintModel(model);
      break;
    }

    // Prepare for another round.
    exp_num++;
    process.push_back({ experiment, oid });
  }
  delete solver;
}

double optimum(Solver& solver, vec<EvalExp>& history,
               double opt) {
  ExpGenerator gen(m.game(), solver, history);
  vec<Experiment> options = gen.All();

  // Lower bound check
  auto models = solver.NumOfModels();
  if (models == 1) {
    assert(!history.empty());
    auto last = history.back();
    auto final_out = last.exp.type().final_outcome();
    if (final_out == -1 || static_cast<int>(last.outcome_id) == final_out)
      return 0;
    else
      return 1;
  }

  // TODO: projit, najít max parts s final sat, vyradit sat 1
  int last = -1;
  uint maxparts = 0;
  for (uint i = 0; i < options.size(); i++) {
    auto parts = options[i].NumOfSat();
    maxparts = std::max(maxparts, parts);
    if (parts == models) {  // this solves the game -> check for FINAL todo: projit vsechny s parts=models
      last = i;
      auto final_out = options[i].type().final_outcome();
      if (final_out == -1 || options[i].IsSat(final_out)) break;
    }
    if (parts == 1) {
      options[i] = options.back();
      options.pop_back();
      i--;
    }
  }
  if (last > -1) {
    auto final_out = options[last].type().final_outcome();
    if (final_out == -1) return 1;
    else if (not options[last].IsSat(final_out)) return 2;
    else return 2 - (static_cast<double>(1) / models);
  }

  double d = log(models)/log(maxparts);
  if (d > opt) return opt; // non-perspective branch

  // Sort according to the 'minnum' stg
  assert(!options.empty());
  vec<uint> sorted;
  sorted.resize(options.size());
  for (uint i = 0; i < options.size(); i++) sorted[i] = i;
  std::sort(sorted.begin(), sorted.end(), [&](const int a, const int b) {
    return options[a].MaxNumOfModels() < options[b].MaxNumOfModels();
  });
  // printf("FIRST: %s (%u)\n", options[0].pretty().c_str(), options[0].MaxNumOfModels());

  // Recurse down
  Experiment* nej = nullptr;
  for (auto i : sorted) {
    auto& e = options[i];
    double val = 0;
    for (uint i = 0; i < e.type().outcomes().size(); i++) {
      if (!e.IsSat(i)) continue;
      solver.OpenContext();
      auto outcome = e.type().outcomes()[i];
      solver.AddConstraint(outcome.formula, e.params());
      history.push_back({ e, i });
      val += (1 + optimum(solver, history, opt)) * solver.NumOfModels();
      history.pop_back();
      solver.CloseContext();
    }
    val /= models;
    if (val < opt) {
      opt = val;
      nej = &e;
    }
  }
  if (!nej) return opt;
  // printf("\n---\n");
  // for (auto e : history) {
  //   printf("%s : %i\n", e.exp.pretty().c_str(), e.outcome_id);
  // }
  // printf(">>> %s (%.2f)\n", nej->pretty().c_str(), opt);
  return opt;
}

void optimal_mode() {
  print_head("AVERAGE-CASE OPTIMAL STRATEGY");
  Game& game = m.game();
  Solver* solver = get_solver(game.vars().size(), game.constraint());
  vec<EvalExp> history;
  auto r = optimum(*solver, history, 5);
  printf("Average-case expected optimum: %.2f\n", r);
}

void analyze(Solver& solver, vec<EvalExp>& history,
             uint depth, uint& max, uint& sum, uint& num) {
  Game& game = m.game();
  ExpGenerator gen(game, solver, history);
  auto options = gen.All();
  auto x = g_breakerStg(options);
  assert(x < options.size());
  auto experiment = options[x];
  for (uint i = 0; i < experiment.type().outcomes().size(); i++) {
    solver.OpenContext();
    auto outcome = experiment.type().outcomes()[i];
    solver.AddConstraint(outcome.formula, experiment.params());
    bool sat = solver.Satisfiable(), one = false;
    if (sat) one = solver.OnlyOneModel();
    if (one) {
      num += 1;
      printf("\b\b\b\b\b%5u", num);
      fflush(stdout);
      auto finaldepth = outcome.final ? depth : depth + 1;
      sum += finaldepth;
      max = std::max(max, finaldepth);
    } else if (sat) {
      history.push_back({ experiment, i });
      analyze(solver, history, depth + 1, max, sum, num);
      history.pop_back();
    }
    solver.CloseContext();
  }
}

void analyze_mode() {
  print_head("STRATEGY ANALYSIS");
  Game& game = m.game();
  Solver* solver = get_solver(game.vars().size(), game.constraint());
  vec<EvalExp> history;
  uint models = solver->NumOfModels();
  uint max = 0, sum = 0, num = 0;
  printf("Codes found (total %u):     0", models);
  fflush(stdout);

  analyze(*solver, history, 1, max, sum, num);
  delete solver;
  printf("\nWorst-case: %u\n", max);
  printf("Average-case: %.4f (%u/%u)\n",
         static_cast<double>(sum)/models, sum, models);
}

// Parse program arguments with TCLAP library.
void parse_args(int argc, char* argv[]) {
  using namespace TCLAP;
  CmdLine cmd("Code Breaking Game Analyzer blah blah blah", ' ', "0.1");

  vec<string> modes = { "d", "s", "simulation",
                        "a", "analysis", "o", "optimal" };
  ValuesConstraint<string> modeConstraint(modes);
  ValueArg<string> modeArg(
    "m", "mode",
    "Mode of operation. Overview mode is default (d).", false,
    "d", &modeConstraint);

  vec<string> backends = { "picosat", "minisat", "simple" };
  ValuesConstraint<string> backendConstraint(backends);
  ValueArg<string> backendArg(
    "b", "backend",
    "Specifies SAT solver that will be used. Default: picosat.", false,
    "simple", &backendConstraint);

  vec<string> e_stgs, o_stgs;
  string e_man = "", o_man = "";
  for (auto s : strategy::breaker_strategies) {
    e_stgs.push_back(s.first);
    e_man += color::emph + s.first + ": " + color::normal + s.second.first;
  }
  for (auto s : strategy::maker_strategies) {
    o_stgs.push_back(s.first);
    o_man += color::emph + s.first + ": " + color::normal + s.second.first;
  }
  ValuesConstraint<string> o_constr(o_stgs);
  ValueArg<string> o_arg(
    "o", "codemaker",
    "Strategy that selects an outcome, played by the codemaker. "
    "Default: interactive." + o_man, false,
    "interactive", &o_constr);
  ValuesConstraint<string> e_constr(e_stgs);
  ValueArg<string> e_arg(
    "e", "codebreaker",
    "Strategy that selects an experiment, played by the codebreaker. "
    "Default: interactive." + e_man, false,
    "interactive", &e_constr);


  UnlabeledValueArg<std::string> filenameArg(
    "filename",
    "Input file name.", false,
    "", "file name");

  cmd.add(e_arg);
  cmd.add(o_arg);
  cmd.add(backendArg);
  cmd.add(modeArg);
  cmd.add(filenameArg);
  cmd.parse(argc, argv);

  args.filename = filenameArg.getValue();
  args.mode = modeArg.getValue();
  args.backend = backendArg.getValue();
  args.stg_experiment = e_arg.getValue();
  args.stg_outcome = o_arg.getValue();
}

int main(int argc, char* argv[]) {
  srand(time(NULL));
  try {
    parse_args(argc, argv);

    if (!(yyin = fopen(args.filename.c_str(), "r"))) {
      printf("Cannot open %s: %s.\n", args.filename.c_str(), strerror(errno));
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
    printf("[%.2fs]\n", static_cast<double>(t2 - t1)/CLOCKS_PER_SEC);
    t1 = clock();

    g_breakerStg = strategy::breaker_strategies.at(args.stg_experiment).second;
    g_makerStg = strategy::maker_strategies.at(args.stg_outcome).second;
    m.game().Precompute();

    if (args.mode == "d") {
      overview_mode();
    } else if (args.mode == "s" || args.mode == "simulation") {
      try {
        simulation_mode();
      } catch (std::runtime_error&) { }
    } else if (args.mode == "a" || args.mode == "analysis") {
      if (args.stg_experiment == "interactive" ||
          args.stg_experiment == "random") {
        printf("Cannot analyze strategy '%s'. \n", args.stg_experiment.c_str());
        exit(EXIT_FAILURE);
      }
      analyze_mode();
    } else if (args.mode == "o" || args.mode == "optimal") {
      optimal_mode();
    }

    time_overview(t1);
    m.deleteAll();
  } catch (const TCLAP::ArgException &e) {
    printf("Error: %s for arg %s.\n", e.error().c_str(), e.argId().c_str());
  }
  return 0;
}

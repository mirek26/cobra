/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <map>
#include <utility>
#include <string>
#include "./formula.h"
#include "./game.h"
#include "./experiment.h"
#include "./common.h"
#include "./parser.h"

#ifndef COBRA_SRC_STRATEGY_H_
#define COBRA_SRC_STRATEGY_H_

namespace strategy {
  namespace breaker {
    uint interactive(vec<Experiment>& options);
    uint random(vec<Experiment>& options);
    uint parts(vec<Experiment>& options);
    uint max_models(vec<Experiment>& options);
    uint exp_models(vec<Experiment>& options);
    uint ent_models(vec<Experiment>& options);
    uint min_fixed(vec<Experiment>& options);
    uint exp_fixed(vec<Experiment>& options);
  }

  namespace maker {
    uint interactive(Experiment& option);
    uint random(Experiment& option);
    uint models(Experiment& option);
    uint fixed(Experiment& option);
  }

  const std::map<string,
                 std::pair<string, std::function<uint(vec<Experiment>&)>>>
    breaker_strategies = {
    { "interactive",
    { "Asks the user which experiment to perform next.",
       breaker::interactive }},

    { "random",
    { "Picks the next experiment by random.",
       breaker::random }},

    { "max-models",
    { "Minimizes the worst-case number of remaining codes in the next step.",
      breaker::max_models }},

    { "exp-models",
    { "Minimizes the expected number of remaining codes in the next step. ",
      breaker::exp_models }},

    { "ent-models",
    { "Maximizes the entropy of numbers of remaining codse.",
      breaker::ent_models }},

    { "parts",
    { "Selects the experiment with maximal number of possible outcomes.",
      breaker::parts }},

    { "min-fixed",
    { "Maximizes the worst-case number of fixed variables in the next step.",
      breaker::min_fixed }},

    { "exp-fixed",
    { "Maximizes the expected number of fixed variables in the next step.",
      breaker::exp_fixed }},
    };

  const std::map<string, std::pair<string, std::function<uint(Experiment&)>>>
    maker_strategies = {
    { "interactive",
    { "Asks the user what the outcome of the experiment is.",
      maker::interactive }},

    { "random",
    { "Picks the outcome of the experiment by random.",
      maker::random }},

    { "models",
    { "Maximizes the number of remaining codes.",
      maker::models }},

    { "fixed",
    { "Minimizes the number of fixed variables.",
      maker::fixed }} };

}  // namespace strategy

#endif  // COBRA_SRC_STRATEGY_H_

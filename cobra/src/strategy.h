/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <map>
#include "formula.h"
#include "game.h"
#include "experiment.h"
#include "common.h"
#include "parser.h"

namespace strategy {
  namespace breaker {
    uint interactive(vec<Option>& options);
    uint random(vec<Option>& options);
    uint parts(vec<Option>& options);
    uint min_num(vec<Option>& options);
    uint entropy(vec<Option>& options);
    uint exp_num(vec<Option>& options);
    uint fixed(vec<Option>& options);
  }

  namespace maker {
    uint interactive(Option& option);
    uint random(Option& option);
    uint max_num(Option& option);
    uint fixed(Option& option);
  }

  const std::map<string, std::pair<string, std::function<uint(vec<Option>&)>>>
    breaker_strategies = {
    { "interactive",
    { "Asks the user which experiment to perform next.",
       breaker::interactive }},

    { "random",
    { "Picks the next experiment by random.",
       breaker::random }},

    { "minnum",
    { "Minimizes the worst-case number of remaining codes in the next step.",
      breaker::min_num }},

    { "expnum",
    { "Minimizes the expected number of remaining codes in the next step. ",
      breaker::exp_num }},

    { "entropy",
    { "Maximizes the entropy of numbers of remaining codse.",
      breaker::entropy }},

    { "parts",
    { "Selects the experiment with maximal number of possible outcomes.",
      breaker::parts }},

    { "fixed",
    { "Maximizes the worst-case number of fixed variables in the next step.",
      breaker::fixed }},
    };

  const std::map<string, std::pair<string, std::function<uint(Option&)>>>
    maker_strategies = {
    { "interactive",
    { "Asks the user what the outcome of the experiment is.",
      maker::interactive }},

    { "random",
    { "Picks the outcome of the experiment by random.",
      maker::random }},

    { "maxnum",
    { "Maximizes the number of remaining codes.",
      maker::max_num }},

    { "fixed",
    { "Minimizes the number of fixed variables.",
      maker::fixed }} };

}
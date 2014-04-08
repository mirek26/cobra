/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <map>
#include "formula.h"
#include "game.h"
#include "experiment.h"
#include "common.h"
#include "parser.h"

namespace Strategy {

  uint breaker_interactive(vec<ExperimentSpec>& options, Game& game);
  uint maker_interactive(ExperimentSpec& option, Game&);

  uint breaker_random(vec<ExperimentSpec>& options, Game& game);
  uint maker_random(ExperimentSpec& option, Game&);

  const std::map<string, std::function<uint(vec<ExperimentSpec>&, Game&)>> breaker_strategies =
    { { "interactive", breaker_interactive },
      { "random", breaker_random } };

  const std::map<string, std::function<uint(ExperimentSpec&, Game&)>> maker_strategies =
    { { "interactive", maker_interactive },
      { "random", maker_random } };


}
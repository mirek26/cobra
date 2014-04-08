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

  uint breaker_interactive(vec<Option>& options, Game& game);
  uint maker_interactive(Option& option, Game&);

  uint breaker_random(vec<Option>& options, Game& game);
  uint maker_random(Option& option, Game&);

  uint breaker_parts(vec<Option>& options, Game&);
  uint breaker_max(vec<Option>& options, Game&);
  uint breaker_entropy(vec<Option>& options, Game&);
  uint breaker_exp(vec<Option>& options, Game&);
  uint breaker_fixed(vec<Option>& options, Game&);


  const std::map<string, std::function<uint(vec<Option>&, Game&)>> breaker_strategies =
    { { "interactive", breaker_interactive },
      { "random", breaker_random },
      { "max", breaker_max },
      { "exp", breaker_exp },
      { "entropy", breaker_entropy },
      { "parts", breaker_parts },
      { "fixed", breaker_fixed },
    };

  const std::map<string, std::function<uint(Option&, Game&)>> maker_strategies =
    { { "interactive", maker_interactive },
      { "random", maker_random } };


}
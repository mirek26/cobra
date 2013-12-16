/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "parser.h"
#include "formula.h"
#include "game.h"

Experiment* g_e;
CnfFormula* g_init;

void Game::setVariables(VariableSet* vars) {
  variables_ = vars;
  for (auto var: *variables_) {
    var->orig(true);
  }
}

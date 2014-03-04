/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "parser.h"
#include "formula.h"
#include "experiment.h"
#include "game.h"

// Experiment* g_e;
// CnfFormula* g_init;

// void Game::setVariables(VariableList* vars) {
//   variables_ = vars;
//   for (auto var: *variables_) {
//     var->orig(true);
//   }
//}

void Game::declareVariable(Variable*) {

}

void Game::declareVariables(VariableList*) {

}

void Game::addRestriction(Formula*) {

}

Formula* Game::initialRestrictions() {
  return init_;
}

void Game::addMapping(std::string, VariableList*) {

}

Experiment* Game::addExperiment(std::string name, int num_params) {
  auto e = new Experiment(name, num_params);
  experiments_.push_back(e);
  return e;
}

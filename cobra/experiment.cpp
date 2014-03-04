/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include <exception>
#include <cassert>

#include "util.h"
#include "formula.h"
#include "game.h"

#include "experiment.h"

void Experiment::addOutcome(std::string name, Formula* outcome) {
  outcomes_names_.push_back(name);
  outcomes_->push_back(outcome);
}

void Experiment::paramsDistinct(std::vector<int>* list) {
  // TODO: nekam to uloz
  delete list;
}

void Experiment::paramsSorted(std::vector<int>* list) {
  // TODO: nekam to uloz
  delete list;
}


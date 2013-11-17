/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include "ast-manager.h"
#include "formula.h"
#include "util.h"

#ifndef COBRA_GAME_H_
#define COBRA_GAME_H_

class Parametrization: public Construct {
 public:
  Parametrization()
      : Construct(0) { }

  virtual Construct* child(uint nth) {
    assert(false);
  }

  virtual void set_child(uint nth, Construct* value) {
    assert(false);
  }

  virtual std::string name() {
    return "Construct.";
  };
};

class Experiment: public Construct {
  std::string name_;
  Parametrization* param_;
  FormulaVec outcomes_;

 public:
  Experiment()
      : Construct(0) { }

  Experiment(std::string name, Parametrization* param, FormulaVec* outcomes)
      : Construct(0),
        name_(name),
        param_(param) {
    outcomes_.insert(outcomes_.begin(), outcomes->begin(), outcomes->end());
    delete outcomes;
  }

  virtual uint child_count() {
    return outcomes_.size();
  }

  virtual Construct* child(uint nth) {
    assert(nth < outcomes_.size());
    return outcomes_[nth];
  };

  virtual void set_child(uint nth, Construct* value) {
    assert(false);
  }

  virtual std::string name() {
    return "Experiment " + name_;
  };
};


#endif   // COBRA_GAME_H_
/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <cassert>
#include <vector>
#include "parser.h"
#include "formula.h"
#include "util.h"

#ifndef COBRA_GAME_H_
#define COBRA_GAME_H_

class Parametrization: public Construct, public std::vector<VariableSet*> {
 public:
  Parametrization()
      : Construct(0) { }

  virtual uint child_count() {
    return size();
  }

  virtual Construct* child(uint nth) {
    assert(nth < size());
    return at(nth);
  }

  virtual void set_child(uint nth, Construct* value) {
    assert(false);
  }

  virtual std::string name() {
    return "Parametrization";
  };
};

class Experiment: public Construct {
  std::string name_;
  Parametrization* param_;
  FormulaList* outcomes_;

 public:
  Experiment()
      : Construct(2) { }

  Experiment(std::string name, Parametrization* param, FormulaList* outcomes)
      : Construct(2),
        name_(name),
        param_(param),
        outcomes_(outcomes) { }

  virtual Construct* child(uint nth) {
    switch (nth) {
      case 0: return param_;
      case 1: return outcomes_;
      default: assert(false);
    }
  };

  virtual std::string name() {
    return "Experiment " + name_;
  };
};


#endif   // COBRA_GAME_H_
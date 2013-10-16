/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cassert>
#include <vector>
#include <string>
#include "./util.h"

#ifndef COBRA_FORMULA_H_
#define COBRA_FORMULA_H_

class Formula;
class Variable;
class OrOperator;
class AndOperator;
class NotOperator;
Variable* newAuxVar();

typedef std::vector<Formula*> FormulaVec;

// Base class for AST node
class Construct {
  const int kChildCount;

 public:
  explicit Construct(int childCount)
      : kChildCount(childCount) { }

  virtual uint child_count() { return kChildCount; }
  virtual Construct* child(uint nth) = 0;
  virtual void set_child(uint nth, Construct* value) { assert(child(nth) == value); }
  virtual std::string name() = 0;
  virtual void dump(int indent = 0);

  virtual Construct* Simplify() {
    for (uint i = 0; i < child_count(); ++i) {
      set_child(i, child(i)->Simplify());
    }
    return this;
  }
};

// Prepositional formula
class Formula: public Construct {
  // variable that is equivalent to this subformula,
  // for Tseitin transformation
  Variable* tseitin_var_;

 public:
  explicit Formula(int childCount)
      : Construct(childCount) { }

  Formula* tseitin_var();
  virtual void TseitinTransformation(FormulaVec& clauses) = 0;
  virtual bool isSimple() { return false; }
  virtual AndOperator* ToCnf();
};

/*
 * Base class for any n-ary operator. Abstract.
 */
class NaryOperator: public Formula {
 protected:
  FormulaVec children_;

 public:
  NaryOperator()
      : Formula(1) {}

  NaryOperator(Formula* left, Formula* right)
      : Formula(1) {
    addChild(left);
    addChild(right);
  }

  explicit NaryOperator(FormulaVec* list)
      : Formula(1) {
    children_.insert(children_.end(), list->begin(), list->end());
    delete list;
  }

  explicit NaryOperator(FormulaVec& list)
      : Formula(1) {
    children_.insert(children_.end(), list.begin(), list.end());
  }

  void addChild(Formula* child) {
    children_.push_back(child);
  }

  void addChilds(FormulaVec childs) {
    children_.insert(children_.end(), childs.begin(), childs.end());
  }

  void removeChild(int nth) {
    children_[nth] = children_.back();
    children_.pop_back();
  }

  virtual Construct* child(uint nth) {
    assert(nth < children_.size());
    return children_[nth];
  }

  virtual uint child_count() {
    return children_.size();
  }
};

// n-ary operator AND
class AndOperator: public NaryOperator {
 public:
  AndOperator()
      : NaryOperator() { }
  AndOperator(Formula *left, Formula* right)
      : NaryOperator(left, right) { }
  explicit AndOperator(FormulaVec* list)
      : NaryOperator(list) { }
  explicit AndOperator(FormulaVec list)
      : NaryOperator(list) { }

  virtual std::string name() {
    return "AndOperator";
  }

  virtual Construct* Simplify();
  virtual void TseitinTransformation(FormulaVec& clauses);
};

// n-ary operator AND
class OrOperator: public NaryOperator {
 public:
  OrOperator(Formula *left, Formula* right)
      : NaryOperator(left, right) {}
  explicit OrOperator(FormulaVec* list)
      : NaryOperator(list) {}
  explicit OrOperator(FormulaVec list)
      : NaryOperator(list) {}

  virtual std::string name() {
    return "OrOperator";
  }

  virtual Construct* Simplify();
  virtual void TseitinTransformation(FormulaVec& clauses);
};

// At least value_ children are satisfied
class AtLeastOperator: public NaryOperator {
  int value_;

 public:
  AtLeastOperator(int value, FormulaVec* list)
      : NaryOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "AtLeastOperator(" + to_string(value_) + ")";
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
  AndOperator* Expand();
};

// At most value_ children are satisfied
class AtMostOperator: public NaryOperator {
  int value_;

 public:
  AtMostOperator(int value, FormulaVec* list)
      : NaryOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "AtMostOperator(" + to_string(value_) + ")";
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
  AndOperator* Expand();
};

// Exactly value_ of children are satisfied
class ExactlyOperator: public NaryOperator {
  int value_;

 public:
  ExactlyOperator(int value, FormulaVec* list)
      : NaryOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "ExactlyOperator(" + to_string(value_) + ")";
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
  AndOperator* Expand();
};

// Equivalence aka iff
class EquivalenceOperator: public Formula {
  Formula* left_;
  Formula* right_;

 public:
  EquivalenceOperator(Formula* left, Formula* right)
      : Formula(2),
        left_(left),
        right_(right) { }

  virtual std::string name() {
    return "EquivalenceOperator";
  }

  virtual Construct* child(uint nth) {
    assert(nth < child_count());
    switch (nth) {
      case 0: return left_;
      case 1: return right_;
      default: assert(false);
    }
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
};

// Implication
class ImpliesOperator: public Formula {
  Formula* left_;
  Formula* right_;

 public:
  ImpliesOperator(Formula* premise, Formula* consequence)
      : Formula(2),
        left_(premise),
        right_(consequence) { }

  virtual std::string name() {
    return "ImpliesOperator";
  }

  virtual Construct* child(uint nth) {
    assert(nth < child_count());
    switch (nth) {
      case 0: return left_;
      case 1: return right_;
      default: assert(false);
    }
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
};

// Simple negation.
class NotOperator: public Formula {
  Formula* child_;
 public:
  explicit NotOperator(Formula* child)
      : Formula(1),
        child_(child) { }

  virtual std::string name() {
    return "NotOperator";
  }

  virtual Construct* child(uint nth) {
    assert(nth == 0);
    return child_;
  }

  virtual bool isSimple() {
    return child_->isSimple();
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
};

// Just a variable, the only possible leaf
class Variable: public Formula {
  std::string ident_;

 public:
  explicit Variable(std::string ident)
      : Formula(0),
        ident_(ident) { }

  virtual std::string name() {
    return "Variable " + ident_;
  }

  virtual Construct* child(uint nth) {
    assert(false);
  }

  virtual bool isSimple() {
    return true;
  }

  virtual void TseitinTransformation(FormulaVec& clauses) {
    // ok.
  }

};

// TODO: remove this
extern Formula* f;

#endif  // COBRA_FORMULA_H_

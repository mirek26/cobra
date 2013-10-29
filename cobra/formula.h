/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cassert>
#include <vector>
#include <map>
#include <string>
#include "util.h"
#include "ast-manager.h"

#ifndef COBRA_FORMULA_H_
#define COBRA_FORMULA_H_

class Formula;
class Variable;
class OrOperator;
class AndOperator;
class NotOperator;
typedef std::vector<Formula*> FormulaVec;

/* Global AstManager. You should never create a ast node directly,
 * always use m.get<Type>(...).
 */
extern AstManager m;

/* Prepositional formula, base class for
 * Derived classes:
 *  - NaryOperator - base class for associative n-ary operators
 *    - AndOperator - logical conjunction
 *    - OrOperator - logical disjunction
 *    - MacroOperator - syntax sugar - expansion can lead to exponential blowup
 *      - AtLeastOperator - at least k of n formulas must be true
 *      - AtMostOperator - at most k of n formulas must be true
 *      - ExactlyOperator - exactly k of n formulas must be true
 *  - EquivalenceOperator - logical equivalence (binary, symmetric)
 *  - ImpliesOperator - logical implication (binary)
 *  - NotOperator - negation of any other formula
 *  - Variable - the only possible leaf of the formula tree
 *  TODO: how about true/false leaves?
 */
class Formula: public Construct {
  /* variable (possibly negated) that is equivalent to this subformula,
   * used for Tseitin transformation
   */
  Variable* tseitin_var_ = nullptr;

 public:
  /*
   */
  explicit Formula(int childCount)
      : Construct(childCount) { }

  /* Getter function for tseitin_var_. If tseitin_var_ was not used yet,
   * a new variable will be created.
   */
  virtual Formula* tseitin_var();

  /* Negate the formula. Equivalent to m.get<NotOperator>(this), except for
   * NotOperator nodes, for which it just returns the child (and thus avoids
   * having two NotOperator nodes under each other).
   */
  virtual Formula* neg();

  /* Dump this subtree in the same format as Construct::dump, but also
   * prints information about tseitin_var_.
   */
  virtual void dump(int indent = 0);

  /* Returns formula as a string. If utf8 is set to true, special math symbols
   * for conjunction, disjunction, implication etc. will be used.
   */
  virtual std::string pretty(bool utf8 = true) = 0;

  /* Returns true for variables or a negations of a variable, false otherwise.
   */
  virtual bool isLiteral() { return false; }

  /* Tseitin transformation - used for conversion to CNF.
   * Capures the relationsships between this node and its children as clauses
   * that are added to the vector 'clauses'. Recursively calls the same
   * on all children.
   */
  virtual void TseitinTransformation(FormulaVec& clauses) = 0;

  /* Converts the formula to CNF using Tseitin transformation.
   * All children of the returned AndOperator are guarenteed to be
   * OrOperators and its children are literals.
   */
  virtual AndOperator* ToCnf();
};

/******************************************************************************
 * Base class for associative n-ary operator. Abstract.
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

  void addChildren(FormulaVec& children) {
    children_.insert(children_.end(), children.begin(), children.end());
  }

  void removeChild(int nth) {
    children_[nth] = children_.back();
    children_.pop_back();
  }

  virtual Construct* child(uint nth) {
    assert(nth < children_.size());
    return children_[nth];
  }

  FormulaVec& children() {
    return children_;
  }

  virtual uint child_count() {
    return children_.size();
  }

 protected:
  std::string pretty_join(std::string sep, bool utf8) {
    if (children_.empty()) return "()";
    std::string s = "(" + children_.front()->pretty(utf8);
    for (auto it = std::next(children_.begin()); it != children_.end(); ++it) {
      s += sep;
      s += (*it)->pretty(utf8);
    }
    s += ")";
    return s;
  }
};

/******************************************************************************
 * Logical conjunctions - n-ary, associative.
 */
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

  virtual std::string pretty(bool utf8 = true) {
    return pretty_join(utf8 ? " ∧ " : " & ", utf8);
  }

  virtual Construct* Simplify();
  virtual void TseitinTransformation(FormulaVec& clauses);
};

/******************************************************************************
 * Logical disjunction - n-ary, associative.
 */
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

  virtual std::string pretty(bool utf8 = true) {
    return pretty_join(utf8 ? " ∨ " : " | ", utf8);
  }

  virtual Construct* Simplify();
  virtual void TseitinTransformation(FormulaVec& clauses);
};

/******************************************************************************
 * Base class syntax sugar like AtLeast/AtMost/ExactlyOperator.
 * Expansion of these operator causes exponential blowup.
 */
class MacroOperator: public NaryOperator {
 protected:
  AndOperator* expanded_;
 public:
  explicit MacroOperator(FormulaVec* list);

  virtual Formula* tseitin_var();

  void ExpandHelper(int size, bool negate);
  virtual AndOperator* Expand() = 0;
  virtual void TseitinTransformation(FormulaVec& clauses);
};

/******************************************************************************
 *
 */
class AtLeastOperator: public MacroOperator {
  int value_;

 public:
  AtLeastOperator(int value, FormulaVec* list)
      : MacroOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "AtLeastOperator(" + to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true) {
    return "AtLeast-" + to_string(value_) + pretty_join(", ", utf8);
  }

  virtual AndOperator* Expand();
};

/******************************************************************************
 *
 */
class AtMostOperator: public MacroOperator {
  int value_;

 public:
  AtMostOperator(int value, FormulaVec* list)
      : MacroOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "AtMostOperator(" + to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true) {
    return "AtMost-" + to_string(value_) + pretty_join(", ", utf8);
  }

  virtual AndOperator* Expand();
};

/******************************************************************************
 *
 */
class ExactlyOperator: public MacroOperator {
  int value_;

 public:
  ExactlyOperator(int value, FormulaVec* list)
      : MacroOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "ExactlyOperator(" + to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true) {
    return "Exactly-" + to_string(value_) + pretty_join(", ", utf8);
  }

  virtual AndOperator* Expand();
};

/******************************************************************************
 * Logical equivalence - binary, symmetric, associative.
 */
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

  virtual std::string pretty(bool utf8 = true) {
    return "(" +
      left_->pretty(utf8) +
      (utf8 ? " ⇔ " : " <-> ") +
      right_->pretty(utf8) +
      ")";
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

/******************************************************************************
 * Logical implication - binary, non-symmetric.
 */
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

  virtual std::string pretty(bool utf8 = true) {
    return "(" +
      left_->pretty(utf8) +
      (utf8 ? " ⇒ " : " -> ") +
      right_->pretty(utf8) +
      ")";
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

/******************************************************************************
 * Negation of a formula.
 */
class NotOperator: public Formula {
  Formula* child_;
 public:
  explicit NotOperator(Formula* child)
      : Formula(1),
        child_(child) { }

  virtual std::string name() {
    return "NotOperator";
  }

  virtual std::string pretty(bool utf8 = true) {
    return (utf8 ? "¬" : "!") + child_->pretty(utf8);
  }

  virtual Construct* child(uint nth) {
    assert(nth == 0);
    return child_;
  }

  virtual Formula* neg() {
    return child_;
  }

  virtual bool isLiteral() {
    return child_->isLiteral();
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
};

/******************************************************************************
 * Prepositional variable. The only possible leaf of the formula AST.
 */
class Variable: public Formula {
  std::string ident_;
  bool generated_;
  int id_;

  static int id_counter_; // initialy 1

 public:
  /* Parameterless constructor creates a generated variable with next
   * available id and named var_ID.
   */
  Variable()
      : Formula(0),
        generated_(true) {
    id_ = id_counter_++;
    ident_ = "var" + to_string(id_);
  }

  explicit Variable(std::string ident)
      : Formula(0),
        ident_(ident),
        generated_(false) {
    id_ = id_counter_++;
  }

  int id() {
    return id_;
  }

  virtual std::string ident() {
    return ident_;
  }

  virtual std::string pretty(bool utf8 = true) {
    return ident_;
  }

  virtual std::string name() {
    return "Variable " + ident();
  }

  virtual Construct* child(uint nth) {
    // this have no children - child() should never be called
    assert(false);
  }

  virtual bool isLiteral() {
    return true;
  }

  virtual void TseitinTransformation(FormulaVec& clauses) {
    // do nothing
  }
};

#endif  // COBRA_FORMULA_H_

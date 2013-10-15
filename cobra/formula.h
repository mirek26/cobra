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
Variable* get_temp();

typedef std::vector<Formula*> FormulaVec;

// Base class for AST node
class Construct {
  const int ChildCount;

 public:
  explicit Construct(int childCount)
      : ChildCount(childCount) { }

  virtual uint getChildCount() {
    return ChildCount;
  }

  virtual ~Construct() { }
  virtual Construct* getChild(uint nth) = 0;
  virtual void setChild(uint nth, Construct* value) {
    // by default, we expect no change
    assert(getChild(nth) == value);
  }

  virtual std::string getName() = 0;
  virtual void dump(int indent = 0);

  virtual Construct* optimize() {
    for (uint i = 0; i < getChildCount(); ++i) {
      setChild(i, getChild(i)->optimize());
    }
    return this;
  }
};

// Prepositional formula
class Formula: public Construct {
  // variable that is equivalent to this subformula,
  // for Tseitin transformation
  Variable* var;

 public:
  explicit Formula(int childCount)
      : Construct(childCount) { }

  Formula* get_var();

  virtual void tseitin(FormulaVec& clauses) = 0;

  virtual bool is_simple() {
    return false;
  }

  virtual AndOperator* to_cnf();
};

/*
 * Base class for any n-ary operator. Abstract.
 */
class NaryOperator: public Formula {
 protected:
  FormulaVec m_list;

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
    m_list.insert(m_list.end(), list->begin(), list->end());
    delete list;
  }

  explicit NaryOperator(FormulaVec& list)
      : Formula(1) {
    m_list.insert(m_list.end(), list.begin(), list.end());
  }

  void addChild(Formula* child) {
    m_list.push_back(child);
  }

  void addChilds(FormulaVec childs) {
    m_list.insert(m_list.end(), childs.begin(), childs.end());
  }

  void removeChild(int nth) {
    m_list[nth] = m_list.back();
    m_list.pop_back();
  }

  virtual Construct* getChild(uint nth) {
    assert(nth < m_list.size());
    return m_list[nth];
  }

  virtual uint getChildCount() {
    return m_list.size();
  }

  AndOperator* expand_prepare(FormulaVec& out_list);

  virtual ~NaryOperator() {
    for (auto& f: m_list) {
      delete f;
    }
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

  virtual std::string getName() {
    return "AndOperator";
  }

  virtual Construct* optimize();

  virtual void tseitin(FormulaVec& clauses);
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

  virtual std::string getName() {
    return "OrOperator";
  }

  virtual Construct* optimize();

  virtual void tseitin(FormulaVec& clauses);
};

// At least m_value children are satisfied
class AtLeastOperator: public NaryOperator {
  int m_value;

 public:
  AtLeastOperator(int value, FormulaVec* list)
      : NaryOperator(list),
        m_value(value) { }

  virtual std::string getName() {
    return "AtLeastOperator(" + to_string(m_value) + ")";
  }

  virtual void tseitin(FormulaVec& clauses);
  AndOperator* expand();
};

// At most m_value children are satisfied
class AtMostOperator: public NaryOperator {
  int m_value;

 public:
  AtMostOperator(int value, FormulaVec* list)
      : NaryOperator(list),
        m_value(value) { }

  virtual std::string getName() {
    return "AtMostOperator(" + to_string(m_value) + ")";
  }

  virtual void tseitin(FormulaVec& clauses);
  AndOperator* expand();
};

// Exactly m_value of children are satisfied
class ExactlyOperator: public NaryOperator {
  int m_value;

 public:
  ExactlyOperator(int value, FormulaVec* list)
      : NaryOperator(list),
        m_value(value) { }

  virtual std::string getName() {
    return "ExactlyOperator(" + to_string(m_value) + ")";
  }

  virtual void tseitin(FormulaVec& clauses);
  AndOperator* expand();
};

// Equivalence aka iff
class EquivalenceOperator: public Formula {
  Formula* m_left;
  Formula* m_right;

 public:
  EquivalenceOperator(Formula* left, Formula* right)
      : Formula(2),
        m_left(left),
        m_right(right) { }

  virtual std::string getName() {
    return "EquivalenceOperator";
  }

  virtual Construct* getChild(uint nth) {
    assert(nth < getChildCount());
    switch (nth) {
      case 0: return m_left;
      case 1: return m_right;
      default: assert(false);
    }
  }

  virtual void tseitin(FormulaVec& clauses);
};

// Implication
class ImpliesOperator: public Formula {
  Formula* m_left;
  Formula* m_right;

 public:
  ImpliesOperator(Formula* premise, Formula* consequence)
      : Formula(2),
        m_left(premise),
        m_right(consequence) { }

  virtual std::string getName() {
    return "ImpliesOperator";
  }

  virtual Construct* getChild(uint nth) {
    assert(nth < getChildCount());
    switch (nth) {
      case 0: return m_left;
      case 1: return m_right;
      default: assert(false);
    }
  }

  virtual void tseitin(FormulaVec& clauses);
};

// Simple negation.
class NotOperator: public Formula {
  Formula* m_child;
 public:
  explicit NotOperator(Formula* child)
      : Formula(1),
        m_child(child) { }

  virtual std::string getName() {
    return "NotOperator";
  }

  virtual Construct* getChild(uint nth) {
    assert(nth == 0);
    return m_child;
  }

  virtual bool is_simple() {
    return m_child->is_simple();
  }

  virtual void tseitin(FormulaVec& clauses);
};

// Just a variable, the only possible leaf
class Variable: public Formula {
  std::string m_ident;

 public:
  explicit Variable(std::string ident)
      : Formula(0),
        m_ident(ident) { }

  virtual std::string getName() {
    return "Variable " + m_ident;
  }

  virtual Construct* getChild(uint nth) {
    assert(false);
  }

  virtual bool is_simple() {
    return true;
  }

  virtual void tseitin(FormulaVec& clauses) {
    // ok.
  }

};

// TODO: remove this
extern Formula* f;

#endif  // COBRA_FORMULA_H_

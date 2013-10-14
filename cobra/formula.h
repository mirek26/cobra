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
 public:
  explicit Formula(int childCount)
      : Construct(childCount) { }
};

/*
 * Base class for any n-ary operator. Abstract.
 */
class NaryOperator: public Formula {
 protected:
  std::vector<Formula*> m_list;

 public:
  NaryOperator(Formula* left, Formula* right)
      : Formula(1) {
    addChild(left);
    addChild(right);
  }

  void addChild(Formula* child) {
    m_list.push_back(child);
  }

  void addChilds(std::vector<Formula*> childs) {
    m_list.insert(m_list.end(), childs.begin(), childs.end());
  }

  void removeChild(int nth) {
    m_list[nth] = m_list.back();
    m_list.pop_back();
  }

  explicit NaryOperator(std::vector<Formula*>* list)
      : Formula(1) {
    m_list.insert(m_list.end(), list->begin(), list->end());
    delete list;
  }

  virtual Construct* getChild(uint nth) {
    assert(nth < m_list.size());
    return m_list[nth];
  }

  virtual uint getChildCount() {
    return m_list.size();
  }

  virtual ~NaryOperator() {
    for (auto& f: m_list) {
      delete f;
    }
  }
};

// n-ary operator AND
class AndOperator: public NaryOperator {
 public:
  AndOperator(Formula *left, Formula* right)
      : NaryOperator(left, right) {}
  explicit AndOperator(std::vector<Formula*>* list)
      : NaryOperator(list) {}

  virtual std::string getName() {
    return "AndOperator";
  }

  virtual Construct* optimize();
};

// n-ary operator AND
class OrOperator: public NaryOperator {
 public:
  OrOperator(Formula *left, Formula* right)
      : NaryOperator(left, right) {}
  explicit OrOperator(std::vector<Formula*>* list)
      : NaryOperator(list) {}

  virtual std::string getName() {
    return "OrOperator";
  }

  virtual Construct* optimize();
};

// At least m_value children are satisfied
class AtLeastOperator: public NaryOperator {
  int m_value;

 public:
  AtLeastOperator(int value, std::vector<Formula*>* list)
      : NaryOperator(list),
        m_value(value) { }

  virtual std::string getName() {
    return "AtLeastOperator(" + to_string(m_value) + ")";
  }
};

// At most m_value children are satisfied
class AtMostOperator: public NaryOperator {
  int m_value;

 public:
  AtMostOperator(int value, std::vector<Formula*>* list)
      : NaryOperator(list),
        m_value(value) { }

  virtual std::string getName() {
    return "AtMostOperator(" + to_string(m_value) + ")";
  }
};

// Exactly m_value of children are satisfied
class ExactlyOperator: public NaryOperator {
  int m_value;

 public:
  ExactlyOperator(int value, std::vector<Formula*>* list)
      : NaryOperator(list),
        m_value(value) { }

  virtual std::string getName() {
    return "ExactlyOperator(" + to_string(m_value) + ")";
  }
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
};

// Implication
class ImpliesOperator: public Formula {
  Formula* m_premise;
  Formula* m_consequence;

 public:
  ImpliesOperator(Formula* premise, Formula* consequence)
      : Formula(2),
        m_premise(premise),
        m_consequence(consequence) { }

  virtual std::string getName() {
    return "ImpliesOperator";
  }

  virtual Construct* getChild(uint nth) {
    assert(nth < getChildCount());
    switch (nth) {
      case 0: return m_premise;
      case 1: return m_consequence;
      default: assert(false);
    }
  }
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
};

// TODO: remove this
extern Formula* f;

#endif  // COBRA_FORMULA_H_

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

// List of prepositional formulas
class ListOfFormulas: public Construct {
  std::vector<Formula*> m_children;

 public:
  virtual ~ListOfFormulas() {
    for (auto& f : m_children) {
      delete f;
    }
  }

  explicit ListOfFormulas(Formula* f)
      : Construct(0) {
    add(f);
  }

  ListOfFormulas(Formula* f1, Formula* f2)
      : Construct(0) {
    add(f1);
    add(f2);
  }

  void add(Formula* f) {
    m_children.push_back(f);
  }

  void add(ListOfFormulas* list) {
    m_children.insert(m_children.end(), list->m_children.begin(), list->m_children.end());
  }

  void clear() {
    m_children.clear();
  }

  void remove(int nth) {
    m_children[nth] = m_children.back();
    m_children.pop_back();
  }

  virtual std::string getName() {
    return "ListOfFormulas";
  }

  virtual uint getChildCount() {
    return m_children.size();
  }

  virtual Construct* getChild(uint nth) {
    assert(nth < m_children.size());
    return m_children[nth];
  }

  virtual void setChild(uint nth, Construct* value) {
    auto f = dynamic_cast<Formula*>(value);
    assert(f);
    m_children[nth] = f;
  }

  std::vector<Formula*>& getVector() {
    return m_children;
  }
};

// Base class for any n-ary operator, abstract
class NaryOperator: public Formula {
 protected:
  ListOfFormulas* m_list;

 public:
  NaryOperator(Formula* left, Formula* right)
      : Formula(1) {
    m_list = new ListOfFormulas(left, right);
  }

  explicit NaryOperator(ListOfFormulas* list)
      : Formula(1) {
    m_list = list;
  }

  virtual Construct* getChild(uint nth) {
    assert(nth < getChildCount());
    return m_list;
  }

  virtual ~NaryOperator() {
    delete m_list;
  }
};

// n-ary operator AND
class AndOperator: public NaryOperator {
 public:
  AndOperator(Formula *left, Formula* right)
      : NaryOperator(left, right) {}
  explicit AndOperator(ListOfFormulas* list)
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
  explicit OrOperator(ListOfFormulas* list)
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
  AtLeastOperator(int value, ListOfFormulas* list)
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
  AtMostOperator(int value, ListOfFormulas* list)
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
  ExactlyOperator(int value, ListOfFormulas* list)
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

#endif  // COBRA_FORMULA_H_

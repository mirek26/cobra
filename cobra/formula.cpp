#include <iostream>
#include <vector>
#include <string>

#ifndef FORMULA_H
#define FORMULA_H

// Base class for AST node 
class Construct {
  const int ChildCount;

public:
  Construct(int childCount)
      : ChildCount(childCount) { }

  virtual int getChildCount() {
    return ChildCount;
  }

  virtual ~Construct() { }
  virtual Construct* getChild(int nth) = 0;
  virtual std::string getName() = 0;
  virtual void dump(int indent = 0) {
    for (int i = 0; i < indent; ++i) {
      std::cout << "   ";
    }
    std::cout << this << ": " << getName() << std::endl;
    for (int i = 0; i < getChildCount(); ++i) {
      getChild(i)->dump(indent + 1);
    }
  }
};

// Prepositional formula
class Formula: public Construct {
public:  
  virtual ~Formula() { }
  Formula(int childCount)
      : Construct(childCount) { }
};

// List of prepositional formulas
class ListOfFormulas: public Construct {
  std::vector<Formula*> m_children;

public:
  virtual ~ListOfFormulas(){
    for (auto& f : m_children) {
      delete f;
    }
  }

  ListOfFormulas(Formula* f)
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

  virtual std::string getName() {
    return "ListOfFormulas";
  }

  virtual int getChildCount() {
    return m_children.size();
  }

  virtual Construct* getChild(int nth) {
    return m_children[nth];
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

  NaryOperator(ListOfFormulas* list)
      : Formula(1) {
    m_list = list;
  }

  virtual Construct* getChild(int nth) {
    return m_list;
  }

  virtual ~NaryOperator() {
    delete m_list;
  }
};

// n-ary operator AND
class AndOperator: public NaryOperator {
public:
  AndOperator(Formula *left, Formula* right): NaryOperator(left, right) {}
  AndOperator(ListOfFormulas* list): NaryOperator(list) {}

  virtual std::string getName() {
    return "AndOperator";
  }  
};

// n-ary operator AND
class OrOperator: public NaryOperator {
public:
  OrOperator(Formula *left, Formula* right): NaryOperator(left, right) {}
  OrOperator(ListOfFormulas* list): NaryOperator(list) {}

  virtual std::string getName() {
    return "OrOperator";
  }
};

// At least m_value children are satisfied
class AtLeastOperator: public NaryOperator {
  int m_value;
public:
  AtLeastOperator(int value, ListOfFormulas* list): 
    NaryOperator(list),
    m_value(value) { }

  virtual std::string getName() {
    return "AtLeastOperator(" + std::to_string(m_value) + ")";
  }
};

// At most m_value children are satisfied
class AtMostOperator: public NaryOperator {
  int m_value;
public:
  AtMostOperator(int value, ListOfFormulas* list): 
    NaryOperator(list), 
    m_value(value) { }

  virtual std::string getName() {
    return "AtMostOperator(" + std::to_string(m_value) + ")";
  }
};

// Exactly m_value of children are satisfied
class ExactlyOperator: public NaryOperator {
  int m_value;
public:
  ExactlyOperator(int value, ListOfFormulas* list): 
    NaryOperator(list), 
    m_value(value) { }

  virtual std::string getName() {
    return "ExactlyOperator(" + std::to_string(m_value) + ")";
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

  virtual Construct* getChild(int nth) {
    switch (nth) {
      case 0: return m_left;
      case 1: return m_right;
    }
    return nullptr;
  }
};

// Implication
class ImpliesOperator: public Formula { 
  Formula* m_premise;
  Formula* m_consequence;
public:
  ImpliesOperator(Formula* premize, Formula* consequence)
      : Formula(2), 
        m_premise(premize),
        m_consequence(consequence) { }

  virtual std::string getName() {
    return "ImpliesOperator";
  }

  virtual Construct* getChild(int nth) {
    switch (nth) {
      case 0: return m_premise;
      case 1: return m_consequence;
    }
    return nullptr;
  }
};

// Simple negation.
class NotOperator: public Formula {
  Formula* m_child;
public:
  NotOperator(Formula* child)
      : Formula(1), 
        m_child(child) { }

  virtual std::string getName() {
    return "NotOperator";
  }

  virtual Construct* getChild(int nth) {
    return m_child;
  }
};

// Just a variable, the only possible leaf
class Variable: public Formula {
  std::string m_ident;

public: 
  Variable(std::string ident)
      : Formula(0),
        m_ident(ident) { }

  virtual std::string getName() {
    return "Variable " + m_ident;
  }

  virtual Construct* getChild(int nth) {
    return nullptr;
  }
};

#endif
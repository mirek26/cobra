#include <iostream>
#include <vector>
#include <string>

#ifndef FORMULA_H
#define FORMULA_H

// Base class for AST node 
class Construct {
public:
  virtual ~Construct() { }
};

// Prepositional formula
class Formula: public Construct {
public:  
  virtual ~Formula() { }
  virtual void dump() = 0;
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

  ListOfFormulas(Formula* f) {
    add(f); 
  }

  ListOfFormulas(Formula* f1, Formula* f2) {
    add(f1);
    add(f2);
  }

  ListOfFormulas(std::vector<Formula*> list) {
    m_children = list;
  }

  void add(Formula* f) {
    m_children.push_back(f);
  }

  virtual void dump(std::string sep = ",") {
    for (auto it = m_children.begin(); it != m_children.end() - 1; ++it) {
      (*it)->dump();
      std::cout << sep;
    }
    m_children.back()->dump();
  }
};

// Base class for any n-ary operator, abstract
class NaryOperator: public Formula {
protected:
  ListOfFormulas* m_children;
 
public:
  NaryOperator(Formula* left, Formula* right) {
    m_children = new ListOfFormulas(left, right);
  }

  NaryOperator(std::vector<Formula*> list) {
    m_children = new ListOfFormulas(list);
  }

  NaryOperator(ListOfFormulas* list) {
    m_children = list;
  }

  virtual ~NaryOperator() {
    delete m_children;
  }
};

// n-ary operator AND
class AndOperator: public NaryOperator {
public:
  AndOperator(Formula *left, Formula* right): NaryOperator(left, right) {}
  AndOperator(std::vector<Formula*> list): NaryOperator(list) {}
  AndOperator(ListOfFormulas* list): NaryOperator(list) {}

  virtual void dump() {
    std::cout << "(";
    m_children->dump(" & ");
    std::cout << ")";
  }  
};

// n-ary operator AND
class OrOperator: public NaryOperator {
public:
  OrOperator(Formula *left, Formula* right): NaryOperator(left, right) {}
  OrOperator(std::vector<Formula*> list): NaryOperator(list) {}
  OrOperator(ListOfFormulas* list): NaryOperator(list) {}

  virtual void dump() {
    std::cout << "(";
    m_children->dump(" | ");
    std::cout << ")";
  }  
};

// At least m_value children are satisfied
class AtLeastOperator: public NaryOperator {
  int m_value;
public:
  AtLeastOperator(int value, ListOfFormulas* list): 
    NaryOperator(list),
    m_value(value) { }

  virtual void dump() {
    std::cout << "AtLeast-" << m_value << "(";
    m_children->dump(", ");
    std::cout << ")";
  }  
};

// At most m_value children are satisfied
class AtMostOperator: public NaryOperator {
  int m_value;
public:
  AtMostOperator(int value, ListOfFormulas* list): 
    NaryOperator(list), 
    m_value(value) { }

  virtual void dump() {
    std::cout << "AtMost-" << m_value << "(";
    m_children->dump(", ");
    std::cout << ")";
  }  
};

// Exactly m_value of children are satisfied
class ExactlyOperator: public NaryOperator {
  int m_value;
public:
  ExactlyOperator(int value, ListOfFormulas* list): 
    NaryOperator(list), 
    m_value(value) { }

  virtual void dump() {
    std::cout << "Exactly-" << m_value << "(";
    m_children->dump(", ");
    std::cout << ")";
  }  
};

// Equivalence aka iff
class EquivalenceOperator: public Formula {
  Formula* m_left;
  Formula* m_right;
public:
  EquivalenceOperator(Formula* left, Formula* right): m_left(left), m_right(right) {
  }

  virtual void dump() {
    std::cout << "(";
    m_left->dump();
    std::cout << "<->";
    m_right->dump();
    std::cout << ")"; 
  }
};

// Implication
class ImpliesOperator: public Formula { 
  Formula* m_predpod;
  Formula* m_dusled;
public:
  ImpliesOperator(Formula* predpok, Formula* dusled): m_predpod(predpok), m_dusled(dusled) {
  }

  virtual void dump() {
    std::cout << "(";
    m_predpod->dump();
    std::cout << "->";
    m_dusled->dump();
    std::cout << ")";
  }
};

// Simple negation.
class NotOperator: public Formula {
  Formula* m_child;
public:
  NotOperator(Formula* child): m_child(child) {
  }

  virtual void dump() {
    std::cout << "!";
    m_child->dump(); 
  }
};

// Just a variable, the only possible leaf
class Variable: public Formula {
  std::string m_ident;

public: 
  Variable(std::string ident): m_ident(ident) {
  }

  virtual void dump() {
    std::cout << m_ident;
  }
};

#endif
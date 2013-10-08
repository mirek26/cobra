#include <vector>

#ifndef FORMULA_H
#define FORMULA_H

class Formula {
 public:
  virtual ~Formula () {}

  //  It's necessary because we need to clone objects without
  // knowing the exact type.
  // virtual Formula *clone () = 0;
  virtual void dump() = 0;
};


// class ListOfFormulas {
//   std::vector<Formula> m_children;

//  public:

// };

class AndOperator: public Formula {
  Formula* m_left, *m_right;
 
public:
  AndOperator(Formula* left, Formula* right):
    m_left(left),
    m_right(right) { }

  virtual ~AndOperator() {
  	delete m_left; 
  	delete m_right;
  }

  virtual void dump() {
  	printf("(");
  	m_left->dump();
  	printf(" & ");
  	m_right->dump();
  	printf(")");
  }
};

class OrOperator: public Formula {
  Formula* m_left, *m_right;
 
public:
  OrOperator(Formula* left, Formula* right):
    m_left(left),
    m_right(right) { }

  virtual ~OrOperator() {
  	delete m_left; 
  	delete m_right;
  }

  virtual void dump() {
  	printf("(");
  	m_left->dump();
  	printf(" | ");
  	m_right->dump();
  	printf(")");
  }
};

class Variable: public Formula {
  char name;

public: 
  Variable(char v) {
  	name = v;
  }

  virtual void dump() {
  	printf("var_%c", name);
  }
};

// class AtLeastOperator

// class AtMostOperator

// class ExactlyOperator

// class Variable

// class ImpliesOperator

// class IffOperator

// class NegOperator

#endif



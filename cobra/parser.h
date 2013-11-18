/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include <exception>
#include "util.h"

#ifndef COBRA_AST_MANAGER_H_
#define COBRA_AST_MANAGER_H_

class Variable;
class VariableSet;
class Formula;
class Experiment;

// Base class for AST node
class Construct {
  const int kChildCount;

 public:
  explicit Construct(int childCount)
      : kChildCount(childCount) { }

  virtual ~Construct() { }
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

template<class C>
class VectorConstruct: public Construct, public std::vector<C> {
 public:
  VectorConstruct(): Construct(0) { }

  virtual uint child_count() {
    return this->size();
  }

  virtual Construct* child(uint nth) {
    assert(nth < this->size());
    return this->at(nth);
  }
};

class ParserException: public std::exception {
  std::string what_;

 public:
  explicit ParserException(std::string what) {
    what_ = what;
  }

  virtual const char* what() const throw() {
    return what_.c_str();
  }
};

class Parser {
  std::vector<Construct*> nodes_;
  std::map<std::string, Variable*> variables_;
  Construct* last_;
  Formula* init_;
  VariableSet* vars_;

 public:
  /* Create a new node of type T; call the constructor with parameters ts.
   * This just calls a private get method with the itenity<T> as the first argument,
   * which can be easily overloaded for different types (e.g., for Variable and string).
   */
  template<class T, typename... Ts>
  T* get(const Ts&... ts) {
    return get(identity<T>(), ts...);
  }

  template<class T, class R>
  T* get(std::initializer_list<R> l) {
    return get(identity<T>(), l);
  }

  void deleteAll() {
    for (auto& n: nodes_) delete n;
    nodes_.clear();
  }

  Formula* init() {
    return init_;
  }

  VariableSet* vars() {
    return vars_;
  }

  void setInit(Formula* init) {
    init_ = init;
  }

  void setVars(VariableSet* vars);
  void addExp(Experiment* exp);

 private:
  /* Generic template for a get method, which creates a new node.
   * It just calls the constructor with given parameters (ts) and stores the
   * created object to nodes_ vector.
   */
  template<typename T, typename... Ts>
  T* get(identity<T>, const Ts&... ts) {
    T* node = new T(ts...);
    nodes_.push_back(node);
    last_ = node;
    return node;
  }

  /* Overloaded instance of get method for a Variable and string as the only
   * argument for the constructor.
   * If first looks into the variable map and creates a new one only if
   * it isn't there yet. This ensures that we don't create more than one node
   * for the same variable.
   */
  Variable* get(identity<Variable>, const std::string& ident);
};

#endif   // COBRA_AST_MANAGER_H_
/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include <exception>
#include <cassert>
#include "common.h"
#include "game.h"

#ifndef COBRA_PARSER_H
#define COBRA_PARSER_H

class Formula;
class Variable;
class Formula;
class ExpType;
class Game;
class PicoSolver;

class ParserException: public std::exception {
  string what_;

 public:
  explicit ParserException(string what) {
    what_ = what;
  }

  virtual const char* what() const throw() {
    return what_.c_str();
  }
};

class Parser {
  vec<Formula*> nodes_;
  std::map<string, Variable*> variables_;
  Formula* last_;

  // auxiliary structures for parsing
  Formula* only_formula_;
  ExpType* last_experiment_;

  Game game_;

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

  void input_assert(bool value, string error_message) {
    if (!value) {
      throw ParserException(error_message);
    }
  }

  void deleteAll();

  void reset() {
    Game g;
    game_ = g;
  }

  template <class T>
  Formula* OnAssocOp(Formula* f1, Formula* f2) {
    auto t = dynamic_cast<T*>(f1);
    if (t) {
      t->addChild(f2);
      return t;
    } else {
      return get<T>({ f1, f2 });
    }
  }

  Game& game() { return game_; }

  Formula* only_formula() { return only_formula_; }
  void set_only_formula(Formula* f) { only_formula_ = f; }

  ExpType* last_experiment() { return last_experiment_; }
  void set_last_experiment(ExpType* e) { last_experiment_ = e; }

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
  // Variable* get(identity<Variable>,
  //               const string& ident);

};

#endif   // COBRA_PARSER_H
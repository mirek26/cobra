/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <vector>
#include <map>
#include <exception>
#include <cassert>
#include <string>
#include "./common.h"
#include "./game.h"

#ifndef COBRA_SRC_PARSER_H_
#define COBRA_SRC_PARSER_H_

class Formula;
class Variable;
class Formula;
class ExpType;
class Game;

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
  /**
   * Creates a new node of type T; call the constructor with parameters ts.
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

  /**
   * Asserts something about input during semantical analysis.
   */
  void input_assert(bool value, string error_message) {
    if (!value) {
      throw ParserException(error_message);
    }
  }

  /**
   * Frees all created nodes.
   */
  void deleteAll();

  /**
   * Resets the game. For debugging purposes only.
   */
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
  /**
   * Generic template for a get method, which creates a new node.
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
};

#endif  // COBRA_SRC_PARSER_H_

/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <cstdio>
#include <cassert>
#include <vector>
#include <unordered_set>
#include <map>
#include <string>
#include <initializer_list>
#include <bliss/graph.hh>
#include "common.h"
#include "parser.h"
#include "pico-solver.h"

#ifndef COBRA_FORMULA_H_
#define COBRA_FORMULA_H_

class Formula;
class Variable;
class OrOperator;
class AndOperator;
class NotOperator;

extern Parser m;

/**
 * Base class for representation of a parametrized propositional formula
 * as a tree. Derived classes are:
 *  - Variable - represents a propositional variable; has no childs.
 *  - Mapping - defined mapping applied on a parameter, e.g. f($1); has no childs.
 *  - NotOperator - negation of any other formula
 *  - ImpliesOperator - logical implication (binary)
 *  - EquivalenceOperator - logical equivalence (binary, symmetric)
 *  - NaryOperator - base class for n-ary operators
 *    - AndOperator - logical conjunction
 *    - OrOperator - logical disjunction
 *    - AtLeastOperator - at least k of n formulas must be true
 *    - AtMostOperator - at most k of n formulas must be true
 *    - ExactlyOperator - exactly k of n formulas must be true
 */

class Formula {
 protected:
  VarId tseitin_var_ = 0;
  bool fixed_;
  bool fixed_value_;
  vec<Formula*> children_;

 public:
  virtual ~Formula() { }

  /**
   * Returns the name of the node, such as "AndOperator".
   */
  virtual string name() = 0;

  /**
   * Gets the vector of node's children.
   */
  const vec<Formula*>& children() const { return children_; }

  virtual uint type_id() = 0; // TODO: remove

  bool fixed() const { return fixed_; }
  bool fixed_value() const { return fixed_value_; }

  /**
   * Returns true for variables or a negations of a variable, false otherwise.
   */
  virtual bool isLiteral() { return false; }

  /**
   * Gets id of the variable used for this node during the transformation to CNF.
   * If id of the variable is not assigned yet, a new id will be created.
   * Note that the value is valid only during an ongoing CNF conversion.
   */
  virtual VarId tseitin_var(PicoSolver& cnf);

  /**
   * Negates the formula. Equivalent to m.get<NotOperator>(this), except for
   * NotOperator nodes, for which it returns the child (and thus avoids
   * having nested NotOperators).
   */
  virtual Formula* neg();

  /**
   * Dump this subtree.
   */
  virtual void dump(int indent = 0);

  /**
   * Returns formula as a string. If utf8 is set to true, special math symbols
   * for conjunction, disjunction, implication etc. will be used.
   */
  virtual string pretty(bool = true, const vec<CharId>* param = nullptr) = 0;

  /**
   * Tseitin transformation - used for conversion to CNF.
   * Capures the relationsships between this node and its children as clauses
   * that are added to the vector 'clauses'. Recursively calls the same
   * on all children.
   */
  virtual void TseitinTransformation(PicoSolver& cnf, bool top) = 0;

  /**
   * Partially evaluates the formula if values of some variables are fixed.
   * It updates fixed_ and fixed_value_ fields of the subtree for a given set
   * of fixed variables. The set of fixed variables should be output
   * of GetFixedVars() method of a SAT solver.
   */
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) = 0;

   /**
   * Adds the formula structure to a symmetry graph. The formula is simplified
   * with the use of fixed_ and fixed_value_ fields. Make sure to call
   * PropagateFixed beforehand.
   */
  virtual void AddToGraph(bliss::Graph& g,
                          const vec<CharId>* params,
                          int parent = -1);

  /**
   * Adds the formula structure to a symmetry graph rooted with a vertex
   * labelled by 'root'. This function creates the root node and calls
   * AddToGraph.
   */
  void AddToGraphRooted(bliss::Graph& g,
                        const vec<CharId>* params,
                        int root);

  /**
   * Returns the number of nodes in the subtree.
   */
  uint Size() const;

  /**
   * Resets ids for Tseitin transofrmation in the subtree. Should be called
   * before a TseitinTransformation.
   */
  void ResetTseitinIds();

  /**
   * Evaluates the formula under a given model.
   */
  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) = 0;

  /**
   * Parses a formula from a string.
   */
  static Formula* Parse(string str);

 protected:
  /**
   * Gets vector of results of tseitin_var() called on all children.
   */
  vec<VarId> tseitin_children(PicoSolver& cnf);

  /**
   * Helper function for pretty. Calls pretty on childs and joins results with 'sep'.
   */
  string pretty_join(string sep, bool utf8, const vec<CharId>* params);
};


/**
 * Base class for any n-ary operator. Provides methods to add children.
 */
class NaryOperator: public Formula {
 public:
  NaryOperator() { }

  explicit NaryOperator(std::initializer_list<Formula*> list) {
    children_.insert(children_.begin(), list.begin(), list.end());
  }

  explicit NaryOperator(vec<Formula*>* list) {
    assert(list);
    children_.insert(children_.begin(), list->begin(), list->end());
    delete list;
  }

  void addChild(Formula* child) {
    children_.push_back(child);
  }

  void addChildren(vec<Formula*>* children) {
    children_.insert(children_.end(), children->begin(), children->end());
    delete children;
  }
};

/**
 * Logical conjunctions - n-ary, associative.
 */
class AndOperator: public NaryOperator {
  uint non_fixed_childs_;

 public:
  AndOperator()
      : NaryOperator() { }
  explicit AndOperator(std::initializer_list<Formula*> list)
      : NaryOperator(list) { }
  explicit AndOperator(vec<Formula*>* list)
      : NaryOperator(list) { }

  virtual uint type_id() { return vertex_type::kAndId; }
  virtual string name() {
    return "AndOperator";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return pretty_join(utf8 ? " ∧ " : " & ", utf8, params);
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);
  virtual void AddToGraph(bliss::Graph& g,
                          const vec<CharId>* params,
                          int parent = -1);
};

/**
 * Logical disjunction - n-ary, associative.
 */
class OrOperator: public NaryOperator {
  uint non_fixed_childs_;

 public:
  OrOperator()
      : NaryOperator() {}
  explicit OrOperator(std::initializer_list<Formula*> list)
      : NaryOperator(list) {}
  explicit OrOperator(vec<Formula*>* list)
     : NaryOperator(list) {}

  virtual uint type_id() { return vertex_type::kOrId; }
  virtual string name() {
    return "OrOperator";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return pretty_join(utf8 ? " ∨ " : " | ", utf8, params);
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);
  virtual void AddToGraph(bliss::Graph& g,
                          const vec<CharId>* params,
                          int parent = -1);
};

/**
 * At least k out of n childs must be satisfied.
 */
class AtLeastOperator: public NaryOperator {
  uint value_;
  uint sat_childs_;

 public:
  AtLeastOperator(uint value, vec<Formula*>* list)
      : NaryOperator(list),
        value_(value) {
    assert(value <= children_.size());
  }

  virtual uint type_id() { return vertex_type::kAtLeastId + 3 * (value_ - sat_childs_); }
  virtual string name() {
    return "AtLeastOperator(" + std::to_string(value_) + ")";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "AtLeast-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);
};

/**
 * At most k out of n childs must be satisfied.
 */
class AtMostOperator: public NaryOperator {
  uint value_;
  uint sat_childs_;

 public:
  AtMostOperator(uint value, vec<Formula*>* list)
      : NaryOperator(list),
        value_(value) {
    assert(value <= children_.size());
  }

  virtual uint type_id() { return vertex_type::kAtMostId + 3 * (value_- sat_childs_); }
  virtual string name() {
    return "AtMostOperator(" + std::to_string(value_) + ")";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "AtMost-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);
};

/**
 * Exactly k out of n childs must be satisfied.
 */
class ExactlyOperator: public NaryOperator {
  uint value_;
  uint sat_childs_;

 public:
  ExactlyOperator(uint value, vec<Formula*>* list)
      : NaryOperator(list),
        value_(value) {
    assert(value <= children_.size());
  }

  virtual uint type_id() { return vertex_type::kExactlyId + 3 * (value_ - sat_childs_); }
  virtual string name() {
    return "ExactlyOperator(" + std::to_string(value_) + ")";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "Exactly-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);
};

/**
 * Logical equivalence - binary, symmetric, associative.
 */
class EquivalenceOperator: public Formula {
 public:
  EquivalenceOperator(Formula* f1, Formula* f2) {
    children_.push_back(f1);
    children_.push_back(f2);
  }

  virtual uint type_id() { return vertex_type::kEquivalenceId; }
  virtual string name() {
    return "EquivalenceOperator";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "(" +
      children_[0]->pretty(utf8, params) +
      (utf8 ? " ⇔ " : " <-> ") +
      children_[1]->pretty(utf8, params) +
      ")";
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);
};

/**
 * Logical implication - binary, non-symmetric.
 */
class ImpliesOperator: public Formula {
 public:
  ImpliesOperator(Formula* premise, Formula* consequence) {
    children_.push_back(premise);
    children_.push_back(consequence);
  }

  virtual uint type_id() { return vertex_type::kImpliesId; }
  virtual string name() {
    return "ImpliesOperator";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "(" +
      children_[0]->pretty(utf8, params) +
      (utf8 ? " ⇒ " : " -> ") +
      children_[1]->pretty(utf8, params) +
      ")";
  }

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void TseitinTransformation(PicoSolver& cnf, bool top);
};

/**
 * Negation of a formula.
 */
class NotOperator: public Formula {
 public:
  explicit NotOperator(Formula* child) {
    children_.push_back(child);
  }

  virtual uint type_id() { return vertex_type::kNotId; }
  virtual string name() {
    return "NotOperator";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return (utf8 ? "¬" : "!") + children_[0]->pretty(utf8, params);
  }

  virtual VarId tseitin_var(PicoSolver& cnf) {
    if (isLiteral()) return -children_[0]->tseitin_var(cnf);
    else return Formula::tseitin_var(cnf);
  }

  virtual Formula* neg() {
    return children_[0];
  }

  virtual bool isLiteral() {
    return children_[0]->isLiteral();
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);
};

/**
 * Prepositional variable.
 */
class Mapping: public Formula {
  string ident_;
  MapId mapping_id_;
  uint param_id_;

 public:
  Mapping(string ident, MapId mapping_id, uint param_id)
      : ident_(ident),
        mapping_id_(mapping_id),
        param_id_(param_id - 1) { // params are internally indexed from 0
  }

  MapId mapping_id() { return mapping_id_; }
  uint param_id() { return param_id_; }

  virtual uint type_id() { return vertex_type::kMappingId; }

  virtual string pretty(bool = true, const vec<CharId>* params = nullptr);

  virtual bool isLiteral() {
    return true;
  }

  /**
   * Gets id of the variable that is the value of the mapping
   * under a given parametrization.
   */
  VarId getValue(const vec<CharId>& params) {
    assert(param_id_ < params.size());
    return m.game().getMappingValue(mapping_id_, params[param_id_]);
  }

  virtual VarId tseitin_var(PicoSolver& cnf) {
    assert(cnf.build_for_params());
    return getValue(*cnf.build_for_params());
  }

  virtual string name() {
    return "Mapping " + pretty();
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params);
  virtual void AddToGraph(bliss::Graph& g,
                          const vec<CharId>* params,
                          int parent = -1);
};

/**
 * Prepositional variable.
 */
class Variable: public Formula {
  string ident_;
  VarId id_;

 public:

  explicit Variable(string ident)
      : ident_(ident) { }

  virtual uint type_id() { return vertex_type::kVariableId; }

  VarId id() { return id_; }
  void set_id(VarId value) { id_ = value; }

  virtual string ident() { return ident_; }

  virtual string pretty(bool = true, const vec<CharId>* = nullptr) {
    return ident_;
  }

  virtual string name() {
    return "Variable " + pretty() + "(" + std::to_string(id_) +")";
  }

  virtual VarId tseitin_var(PicoSolver&) {
    return id_;
  }

  virtual bool isLiteral() {
    return true;
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId>);
  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>*);
  virtual void AddToGraph(bliss::Graph& g,
                          const vec<CharId>* params,
                          int parent = -1);
};

#endif  // COBRA_FORMULA_H_

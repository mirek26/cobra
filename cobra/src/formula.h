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

/* Global Parser object. You should never create a ast node directly,
 * always use m.get<Type>(...).
 */
extern Parser m;

/* Propositional formula.
 * Derived classes (node type id).
 *  - Variable (1) - the only possible leaf of the formula tree
 *  - NotOperator (2) - negation of any other formula
 *  - ImpliesOperator (3) - logical implication (binary)
 *  - EquivalenceOperator (4) - logical equivalence (binary, symmetric)
 *  - Mapping (7) - special construct in parametrized formulas : f($1)
 *  - NaryOperator - base class for n-ary operators
 *    - AndOperator (5) - logical conjunction
 *    - OrOperator (6) - logical disjunction
 *    - AtLeastOperator (8+3k)- at least k of n formulas must be true
 *    - AtMostOperator (9+3k) - at most k of n formulas must be true
 *    - ExactlyOperator (10+3k)- exactly k of n formulas must be true
 */

class Formula {
 protected:
  /* variable (possibly negated) that is equivalent to this subformula,
   * used for Tseitin transformation
   */
  VarId tseitin_var_ = 0;
  bool fixed_;
  bool fixed_value_;

 public:
  const int kChildCount;

  explicit Formula(int childCount)
      : kChildCount(childCount) { }

  virtual ~Formula() { }
  virtual uint child_count() const { return kChildCount; }
  virtual Formula* child(uint) const { assert(false); };
  virtual void set_child(uint nth, Formula* value) { assert(child(nth) == value); }
  virtual string name() = 0;
  virtual uint type_id() = 0;
  bool fixed() const { return fixed_; }
  bool fixed_value() const { return fixed_value_; }

  /* Returns true for variables or a negations of a variable, false otherwise.
   */
  virtual bool isLiteral() { return false; }

  /* Getter function for tseitin_var_. If tseitin_var_ was not used yet,
   * a new variable id will be created.
   */
  virtual VarId tseitin_var(PicoSolver& cnf);

  /* Negate the formula. Equivalent to m.get<NotOperator>(this), except for
   * NotOperator nodes, for which it just returns the child (and thus avoids
   * having two NotOperator nodes under each other).
   */
  virtual Formula* neg();

  /* Dump this subtree in the same format as Formula::dump, but also
   * prints information about tseitin_var_.
   */
  virtual void dump(int indent = 0);

  /* Returns formula as a string. If utf8 is set to true, special math symbols
   * for conjunction, disjunction, implication etc. will be used.
   */
  virtual string pretty(bool = true, const vec<CharId>* param = nullptr) = 0;

  /* Tseitin transformation - used for conversion to CNF.
   * Capures the relationsships between this node and its children as clauses
   * that are added to the vector 'clauses'. Recursively calls the same
   * on all children.
   */
  virtual void TseitinTransformation(PicoSolver& cnf, bool top) = 0;

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) = 0;
  void AddToGraph(bliss::Graph& g,
                          const vec<CharId>* params,
                          int parent = -1);


  static Formula* Parse(string str);

  uint Size() const {
    uint r = 1;
    for (uint i = 0; i < child_count(); ++i) {
      r += child(i)->Size();
    }
    return r;
  }

  void clearTseitinIds() {
    tseitin_var_ = 0;
    for (uint i = 0; i < child_count(); ++i) {
      child(i)->clearTseitinIds();
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) = 0;
};


/******************************************************************************
 * Base class for associative n-ary operator. Abstract.
 */
class NaryOperator: public Formula {
 protected:
  vec<Formula*> children_;

 public:
  NaryOperator()
       : Formula(0) { }

  explicit NaryOperator(std::initializer_list<Formula*> list)
      : Formula(0) {
    children_.insert(children_.begin(), list.begin(), list.end());
  }

  explicit NaryOperator(vec<Formula*>* list)
      : Formula(1) {
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

  void removeChild(int nth) {
    children_[nth] = children_.back();
    children_.pop_back();
  }

  virtual uint child_count() const {
    return children_.size();
  }

  virtual Formula* child(uint nth) const {
    assert(nth < children_.size());
    return children_[nth];
  }

  const vec<Formula*>& children() const {
    return children_;
  }

 protected:
  vec<VarId> tseitin_children(PicoSolver& cnf) {
    vec<VarId> vars(children_.size(), 0);
    std::transform(children_.begin(), children_.end(), vars.begin(), [&](Formula* f){
      return f->tseitin_var(cnf);
    });
    return vars;
  }

  string pretty_join(string sep, bool utf8, const vec<CharId>* params) {
    if (children_.empty()) return "()";
    string s = "(" + children_.front()->pretty(utf8, params);
    for (auto it = std::next(children_.begin()); it != children_.end(); ++it) {
      s += sep;
      s += (*it)->pretty(utf8, params);
    }
    s += ")";
    return s;
  }
};

/******************************************************************************
 * Logical conjunctions - n-ary, associative.
 */
class AndOperator: public NaryOperator {
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

  // virtual Formula* clone() {
  //   // return m.get<AndOperator>(children_->clone());
  // }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    fixed_ = false;
    bool fixed_all = true;
    for (auto c: children_) {
      c->PropagateFixed(fixed, params);
      if (c->fixed() == true && c->fixed_value()== false) {
        fixed_ = true;
        fixed_value_ = false;
        return;
      }
      fixed_all = fixed_all && c->fixed();
    }
    if (fixed_all) {
      fixed_ = true;
      fixed_value_ = true;
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    for (uint i = 0; i < child_count(); ++i) {
      if (!child(i)->Satisfied(model, params)) return false;
    }
    return true;
  }
};

/******************************************************************************
 * Logical disjunction - n-ary, associative.
 */
class OrOperator: public NaryOperator {
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

  // virtual Formula* clone() {
  //   return m.get<OrOperator>(children_->clone());
  // }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    fixed_ = false;
    bool fixed_all = true;
    for (uint i = 0; i < child_count(); ++i) {
      auto c = child(i);
      c->PropagateFixed(fixed, params);
      if (c->fixed() == true && c->fixed_value() == true) {
        fixed_ = true;
        fixed_value_ = true;
        return;
      }
      fixed_all = fixed_all && c->fixed();
    }
    if (fixed_all) {
      fixed_ = true;
      fixed_value_ = false;
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    for (uint i = 0; i < child_count(); ++i) {
      if (child(i)->Satisfied(model, params)) return true;
    }
    return false;
  }
};

/******************************************************************************
 *
 */
class AtLeastOperator: public NaryOperator {
  uint value_;
  uint fixed_childs_;

 public:
  AtLeastOperator(uint value, vec<Formula*>* list)
      : NaryOperator(list),
        value_(value),
        fixed_childs_(0) {
    assert(value <= children_.size());
  }

  virtual uint type_id() { return vertex_type::kAtLeastId + 3 * (value_ - fixed_childs_); }
  virtual string name() {
    return "AtLeastOperator(" + std::to_string(value_) + ")";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "AtLeast-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  // virtual Formula* clone() {
  //   return m.get<AtLeastOperator>(value_, children_->clone());
  // }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    uint t = 0, f = 0;
    for (auto c: children_) {
      c->PropagateFixed(fixed, params);
      if (c->fixed() == true) {
        fixed_childs_ ++;
        if (c->fixed_value()) t++;
        else f++;
      }
    }
    fixed_ = false;
    fixed_childs_ = t;
    if (t >= value_) {
      fixed_ = true;
      fixed_value_ = true;
    } else if (f > child_count() - value_) {
      fixed_ = true;
      fixed_value_ = false;
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    uint sat = 0;
    for (uint i = 0; i < child_count(); ++i) {
      sat += child(i)->Satisfied(model, params);
      if (sat >= value_) return true;
    }
    return false;
  }
};

/******************************************************************************
 *
 */
class AtMostOperator: public NaryOperator {
  uint value_;
  uint fixed_childs_;

 public:
  AtMostOperator(uint value, vec<Formula*>* list)
      : NaryOperator(list),
        value_(value),
        fixed_childs_(0) {
    assert(value <= children_.size());
  }

  virtual uint type_id() { return vertex_type::kAtMostId + 3 * (value_- fixed_childs_); }
  virtual string name() {
    return "AtMostOperator(" + std::to_string(value_) + ")";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "AtMost-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  // virtual Formula* clone() {
  //   return m.get<AtMostOperator>(value_, children_->clone());
  // }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    uint t = 0, f = 0;
    for (auto c: children_) {
      c->PropagateFixed(fixed, params);
      if (c->fixed() == true) {
        fixed_childs_++;
        if (c->fixed_value()) t++;
        else f++;
      }
    }
    fixed_ = false;
    fixed_childs_ = t;
    if (f >= child_count() - value_) {
      fixed_ = true;
      fixed_value_ = true;
    } else if (t > value_) {
      fixed_ = true;
      fixed_value_ = false;
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    uint sat = 0;
    for (uint i = 0; i < child_count(); ++i) {
      sat += child(i)->Satisfied(model, params);
    }
    return sat <= value_;
  }
};

/******************************************************************************
 *
 */
class ExactlyOperator: public NaryOperator {
  uint value_;
  uint fixed_childs_;

 public:
  ExactlyOperator(uint value, vec<Formula*>* list)
      : NaryOperator(list),
        value_(value),
        fixed_childs_(0) {
    assert(value <= children_.size());
  }

  virtual uint type_id() { return vertex_type::kExactlyId + 3 * (value_ - fixed_childs_); }
  virtual string name() {
    return "ExactlyOperator(" + std::to_string(value_) + ")";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "Exactly-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  // virtual Formula* clone() {
  //   return m.get<ExactlyOperator>(value_, children_->clone());
  // }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    uint t = 0, f = 0;
    for (auto c: children_) {
      c->PropagateFixed(fixed, params);
      if (c->fixed() == true) {
        if (c->fixed_value()) t++;
        else f++;
      }
    }
    fixed_ = false;
    fixed_childs_ = t;
    if (t + f == child_count() && t == value_) {
      fixed_ = true;
      fixed_value_ = true;
    } else if (f > child_count() - value_ || t > value_) {
      fixed_ = true;
      fixed_value_ = false;
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    uint sat = 0;
    for (uint i = 0; i < child_count(); ++i) {
      sat += child(i)->Satisfied(model, params);
    }
    return sat == value_;
  }
};

/******************************************************************************
 * Logical equivalence - binary, symmetric, associative.
 */
class EquivalenceOperator: public Formula {
  Formula* left_;
  Formula* right_;

 public:
  EquivalenceOperator(Formula* left, Formula* right)
      : Formula(2),
        left_(left),
        right_(right) { }

  virtual uint type_id() { return vertex_type::kEquivalenceId; }
  virtual string name() {
    return "EquivalenceOperator";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "(" +
      left_->pretty(utf8, params) +
      (utf8 ? " ⇔ " : " <-> ") +
      right_->pretty(utf8, params) +
      ")";
  }

  virtual Formula* child(uint nth) const {
    assert(nth < child_count());
    switch (nth) {
      case 0: return left_;
      case 1: return right_;
      default: assert(false);
    }
  }

  // virtual Formula* clone() {
  //   return m.get<EquivalenceOperator>(left_->clone(), right_->clone());
  // }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    fixed_ = false;
    left_->PropagateFixed(fixed, params);
    right_->PropagateFixed(fixed, params);
    if (left_->fixed() && right_->fixed()) {
      fixed_ = true;
      fixed_value_ = (left_->fixed_value()== right_->fixed_value());
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    return left_->Satisfied(model, params) == right_->Satisfied(model, params);
  }
};

/******************************************************************************
 * Logical implication - binary, non-symmetric.
 */
class ImpliesOperator: public Formula {
  Formula* left_;
  Formula* right_;

 public:
  ImpliesOperator(Formula* premise, Formula* consequence)
      : Formula(2),
        left_(premise),
        right_(consequence) { }

  virtual uint type_id() { return vertex_type::kImpliesId; }
  virtual string name() {
    return "ImpliesOperator";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return "(" +
      left_->pretty(utf8, params) +
      (utf8 ? " ⇒ " : " -> ") +
      right_->pretty(utf8, params) +
      ")";
  }

  virtual Formula* child(uint nth) const {
    assert(nth < child_count());
    switch (nth) {
      case 0: return left_;
      case 1: return right_;
      default: assert(false);
    }
  }

  // virtual Formula* clone() {
  //   return m.get<ImpliesOperator>(left_->clone(), right_->clone());
  // }

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    fixed_ = false;
    left_->PropagateFixed(fixed, params);
    right_->PropagateFixed(fixed, params);
    if (left_->fixed() && left_->fixed_value()== false) { // false -> ?
      fixed_ = true;
      fixed_value_ = true;
    } else if (right_->fixed() && right_->fixed_value()== true) { // ? -> true
      fixed_ = true;
      fixed_value_ = true;
    } else if (left_->fixed() && right_->fixed() &&  // false -> true
               left_->fixed_value()== true && right_->fixed_value()== false) {
      fixed_ = true;
      fixed_value_ = false;
    }
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    return !left_->Satisfied(model, params) || right_->Satisfied(model, params);
  }
};

/******************************************************************************
 * Negation of a formula.
 */
class NotOperator: public Formula {
  Formula* child_;
 public:
  explicit NotOperator(Formula* child)
      : Formula(1),
        child_(child) { }

  virtual uint type_id() { return vertex_type::kNotId; }
  virtual string name() {
    return "NotOperator";
  }

  virtual string pretty(bool utf8 = true, const vec<CharId>* params = nullptr) {
    return (utf8 ? "¬" : "!") + child_->pretty(utf8, params);
  }

  virtual Formula* child(uint nth) const {
    assert(nth == 0);
    return child_;
  }

  virtual VarId tseitin_var(PicoSolver& cnf) {
    if (isLiteral()) return -child_->tseitin_var(cnf);
    else return Formula::tseitin_var(cnf);
  }

  // virtual Formula* clone() {
  //   return m.get<NotOperator>(child_->clone());
  // }

  virtual Formula* neg() {
    return child_;
  }

  virtual bool isLiteral() {
    return child_->isLiteral();
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top);

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    fixed_ = false;
    child_->PropagateFixed(fixed, params);
    fixed_ = child_->fixed();
    fixed_value_ = !child_->fixed_value();
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    return !child_->Satisfied(model, params);
  }
};

class Mapping: public Formula {
  string ident_;
  MapId mapping_id_;
  uint param_id_;

 public:
  Mapping(string ident, MapId mapping_id, uint param_id)
      : Formula(0),
        ident_(ident),
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

  // virtual Formula* clone() {
  //   return m.get<Mapping>(ident_, mapping_id_, param_id_);
  // }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top) {
    if (top) {
      assert(cnf.build_for_params());
      cnf.AddClause({ getValue(*cnf.build_for_params()) });
    }
  }

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>* params) {
    fixed_ = false;
    assert(params);
    if (std::count(fixed.begin(), fixed.end(), getValue(*params))) {
      fixed_ = true;
      fixed_value_ = true;
    } else if (std::count(fixed.begin(), fixed.end(), -getValue(*params))) {
      fixed_ = true;
      fixed_value_ = false;
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId> params) {
    return model[getValue(params)];
  }
};

/******************************************************************************
 * Prepositional variable. The only possible leaf of the formula AST.
 */
class Variable: public Formula {
  string ident_;
  VarId id_;

 public:

  explicit Variable(string ident)
      : Formula(0),
        ident_(ident)
  { }

  virtual uint type_id() { return vertex_type::kVariableId; }

  VarId id() { return id_; }
  void set_id(VarId value) {
    id_ = value;
  }

  virtual string ident() { return ident_; }

  virtual string pretty(bool = true, const vec<CharId>* = nullptr) {
    return ident_;
  }

  virtual string name() {
    return "Variable " + pretty() + "(" + std::to_string(id_) +")";
  }

  virtual Formula* child(uint) const {
    // this have no children - child() should never be called
    assert(false);
  }

  virtual VarId tseitin_var(PicoSolver&) {
    return id_;
  }

  // virtual Formula* clone() {
    // if (variable_substitute_ && variable_substitute_->count(this) > 0) {
    //   return variable_substitute_->at(this);
    // } else if (index_substitute_) {
    //   vec<int> newIndices(indices_);
    //   for (auto& i: newIndices) {
    //     if (index_substitute_->count(i) > 0) {
    //       i = index_substitute_->at(i);
    //     }
    //   }
    //   return m.get<Variable>(ident_, newIndices);
    // } else {
      // return this;
    //}
  // }

  virtual bool isLiteral() {
    return true;
  }

  virtual void TseitinTransformation(PicoSolver& cnf, bool top) {
    if (top) {
      cnf.AddClause({ id_ });
    }
  }

  virtual void PropagateFixed(const vec<VarId>& fixed, const vec<CharId>*) {
    fixed_ = false;
    if (std::count(fixed.begin(), fixed.end(), id_)) {
      fixed_ = true;
      fixed_value_ = true;
    } else if (std::count(fixed.begin(), fixed.end(), -id_)) {
      fixed_ = true;
      fixed_value_ = false;
    }
  }

  virtual bool Satisfied(const vec<bool>& model, const vec<CharId>) {
    assert((unsigned)id_ < model.size());
    return model[id_];
  }
};

#endif  // COBRA_FORMULA_H_

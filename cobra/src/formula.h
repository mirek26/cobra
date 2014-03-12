/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cassert>
#include <vector>
#include <map>
#include <string>
#include <initializer_list>
#include "common.h"
#include "parser.h"
#include "cnf-formula.h"

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

/* Prepositional formula.
 * Derived classes (node type id)
 *  - Variable (1) - the only possible leaf of the formula tree
 *  - NotOperator (2) - negation of any other formula
 *  - ImpliesOperator (3) - logical implication (binary)
 *  - EquivalenceOperator (4) - logical equivalence (binary, symmetric)
 *  - Mapping (7) - special construct in parametrized formulas : f($1)
 *  - NaryOperator - base class for n-ary operators
 *    - AndOperator (5) - logical conjunction
 *    - OrOperator (6) - logical disjunction
 *    - MacroOperator - syntax sugar
 *      - AtLeastOperator (8+3k)- at least k of n formulas must be true
 *      - AtMostOperator (9+3k) - at most k of n formulas must be true
 *      - ExactlyOperator (10+3k)- exactly k of n formulas must be true
 *  TODO: how about true/false leaves?
 */
class Formula {
 protected:
  /* variable (possibly negated) that is equivalent to this subformula,
   * used for Tseitin transformation
   */
  VarId tseitin_var_ = 0;

 public:
  const int kChildCount;
  const int kVariableId = 1,
            kNotId = 2,
            kImpliesId = 3,
            kEquivalenceId = 4,
            kAndId = 5,
            kOrId = 6,
            kMappingId = 7,
            kAtLeastId = 8,
            kAtMostId = 9,
            kExactlyId = 10;

  explicit Formula(int childCount)
      : kChildCount(childCount) { }

  virtual ~Formula() { }
  virtual uint child_count() { return kChildCount; }
  virtual Formula* child(uint) { assert(false); };
  virtual void set_child(uint nth, Formula* value) { assert(child(nth) == value); }
  virtual std::string name() = 0;
  virtual uint node_id() = 0;

  /* Returns true for variables or a negations of a variable, false otherwise.
   */
  virtual bool isLiteral() { return false; }

  /* Getter function for tseitin_var_. If tseitin_var_ was not used yet,
   * a new variable id will be created.
   */
  virtual VarId tseitin_var(std::vector<CharId>* params = nullptr);

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
  virtual std::string pretty(bool = true, std::vector<CharId>* param = nullptr) = 0;

  /* Tseitin transformation - used for conversion to CNF.
   * Capures the relationsships between this node and its children as clauses
   * that are added to the vector 'clauses'. Recursively calls the same
   * on all children.
   */
  virtual void TseitinTransformation(CnfFormula* cnf, bool top) = 0;

  /* Converts the formula to CNF using Tseitin transformation.
   */
  CnfFormula* ToCnf();
  CnfFormula* ToCnf(std::vector<CharId>& param);

  // virtual Formula* clone() = 0;

  //Formula* Substitude(std::map<Variable*, Variable*>& table);

  static Formula* Parse(std::string str);
};




/******************************************************************************
 * Base class for associative n-ary operator. Abstract.
 */
class NaryOperator: public Formula {
 protected:
  std::vector<Formula*> children_;

 public:
  NaryOperator()
       : Formula(0) { }

  explicit NaryOperator(std::initializer_list<Formula*> list)
      : Formula(0) {
    children_.insert(children_.begin(), list.begin(), list.end());
  }

  explicit NaryOperator(std::vector<Formula*>* list)
      : Formula(1) {
    assert(list);
    children_.insert(children_.begin(), list->begin(), list->end());
    delete list;
  }

  void addChild(Formula* child) {
    children_.push_back(child);
  }

  void addChildren(std::vector<Formula*>* children) {
    children_.insert(children_.end(), children->begin(), children->end());
    delete children;
  }

  void removeChild(int nth) {
    children_[nth] = children_.back();
    children_.pop_back();
  }

  virtual uint child_count() {
    return children_.size();
  }

  virtual Formula* child(uint nth) {
    assert(nth < children_.size());
    return children_[nth];
  }

  std::vector<Formula*>& children() {
    return children_;
  }

 protected:
  std::string pretty_join(std::string sep, bool utf8, std::vector<CharId>* params) {
    if (children_.empty()) return "()";
    std::string s = "(" + children_.front()->pretty(utf8, params);
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
  explicit AndOperator(std::vector<Formula*>* list)
      : NaryOperator(list) { }

  virtual uint node_id() { return kAndId; }
  virtual std::string name() {
    return "AndOperator";
  }

  virtual std::string pretty(bool utf8 = true, std::vector<CharId>* params = nullptr) {
    return pretty_join(utf8 ? " ∧ " : " & ", utf8, params);
  }

  // virtual Formula* clone() {
  //   // return m.get<AndOperator>(children_->clone());
  // }

  virtual void TseitinTransformation(CnfFormula* cnf, bool top);
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
  explicit OrOperator(std::vector<Formula*>* list)
     : NaryOperator(list) {}

  virtual uint node_id() { return kOrId; }
  virtual std::string name() {
    return "OrOperator";
  }

  virtual std::string pretty(bool utf8 = true, std::vector<CharId>* params = nullptr) {
    return pretty_join(utf8 ? " ∨ " : " | ", utf8, params);
  }

  // virtual Formula* clone() {
  //   return m.get<OrOperator>(children_->clone());
  // }

  virtual void TseitinTransformation(CnfFormula* cnf, bool top);
};

/******************************************************************************
 * Base class syntax sugar like AtLeast/AtMost/ExactlyOperator.
 * Expansion of these operator causes exponential blowup.
 */
class MacroOperator: public NaryOperator {
 protected:
  AndOperator* expanded_;
 public:
  explicit MacroOperator(std::vector<Formula*>* list);

  //virtual VarId tseitin_var();

  void ExpandHelper(uint size, bool negate);
  virtual AndOperator* Expand() = 0;
  virtual void TseitinTransformation(CnfFormula* cnf, bool top);
};

/******************************************************************************
 *
 */
class AtLeastOperator: public MacroOperator {
  uint value_;

 public:
  AtLeastOperator(uint value, std::vector<Formula*>* list)
      : MacroOperator(list),
        value_(value) {
    assert(value <= children_.size());
  }

  virtual uint node_id() { return kAtLeastId + 3*value_; }
  virtual std::string name() {
    return "AtLeastOperator(" + std::to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true, std::vector<CharId>* params = nullptr) {
    return "AtLeast-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  // virtual Formula* clone() {
  //   return m.get<AtLeastOperator>(value_, children_->clone());
  // }

  virtual AndOperator* Expand();
};

/******************************************************************************
 *
 */
class AtMostOperator: public MacroOperator {
  uint value_;

 public:
  AtMostOperator(uint value, std::vector<Formula*>* list)
      : MacroOperator(list),
        value_(value) {
    assert(value <= children_.size());
  }

  virtual uint node_id() { return kAtMostId + 3*value_; }
  virtual std::string name() {
    return "AtMostOperator(" + std::to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true, std::vector<CharId>* params = nullptr) {
    return "AtMost-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  // virtual Formula* clone() {
  //   return m.get<AtMostOperator>(value_, children_->clone());
  // }

  virtual AndOperator* Expand();
};

/******************************************************************************
 *
 */
class ExactlyOperator: public MacroOperator {
  uint value_;

 public:
  ExactlyOperator(uint value, std::vector<Formula*>* list)
      : MacroOperator(list),
        value_(value) {
    assert(value <= children_.size());
  }

  virtual uint node_id() { return kExactlyId + 3*value_; }
  virtual std::string name() {
    return "ExactlyOperator(" + std::to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true, std::vector<CharId>* params = nullptr) {
    return "Exactly-" + std::to_string(value_) + pretty_join(", ", utf8, params);
  }

  // virtual Formula* clone() {
  //   return m.get<ExactlyOperator>(value_, children_->clone());
  // }

  virtual AndOperator* Expand();
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

  virtual uint node_id() { return kEquivalenceId; }
  virtual std::string name() {
    return "EquivalenceOperator";
  }

  virtual std::string pretty(bool utf8 = true, std::vector<CharId>* params = nullptr) {
    return "(" +
      left_->pretty(utf8, params) +
      (utf8 ? " ⇔ " : " <-> ") +
      right_->pretty(utf8, params) +
      ")";
  }

  virtual Formula* child(uint nth) {
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

  virtual void TseitinTransformation(CnfFormula* cnf, bool top);
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

  virtual uint node_id() { return kImpliesId; }
  virtual std::string name() {
    return "ImpliesOperator";
  }

  virtual std::string pretty(bool utf8 = true, std::vector<CharId>* params = nullptr) {
    return "(" +
      left_->pretty(utf8, params) +
      (utf8 ? " ⇒ " : " -> ") +
      right_->pretty(utf8, params) +
      ")";
  }

  virtual Formula* child(uint nth) {
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

  virtual void TseitinTransformation(CnfFormula* cnf, bool top);
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

  virtual uint node_id() { return kNotId; }
  virtual std::string name() {
    return "NotOperator";
  }

  virtual std::string pretty(bool utf8 = true, std::vector<CharId>* params = nullptr) {
    return (utf8 ? "¬" : "!") + child_->pretty(utf8, params);
  }

  virtual Formula* child(uint nth) {
    assert(nth == 0);
    return child_;
  }

  virtual VarId tseitin_var(std::vector<CharId>* params = nullptr) {
    if (isLiteral()) return -child_->tseitin_var(params);
    else return Formula::tseitin_var(params);
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

  virtual void TseitinTransformation(CnfFormula* cnf, bool top);
};

class Mapping: public Formula {
  std::string ident_;
  MapId mapping_id_;
  uint param_id_;

 public:
  Mapping(std::string ident, MapId mapping_id, uint param_id)
      : Formula(0),
        ident_(ident),
        mapping_id_(mapping_id),
        param_id_(param_id - 1) { // params are internally indexed from 0
  }

  MapId mapping_id() { return mapping_id_; }
  uint param_id() { return param_id_; }

  virtual uint node_id() { return kMappingId; }

  virtual std::string pretty(bool = true, std::vector<CharId>* params = nullptr);

  virtual bool isLiteral() {
    return true;
  }

  VarId getValue(std::vector<CharId>& params) {
    assert(param_id_ < params.size());
    return m.game().getMappingValue(mapping_id_, params[param_id_]);
  }

  virtual VarId tseitin_var(std::vector<CharId>* params = nullptr) {
    assert(params);
    return getValue(*params);
  }

  virtual std::string name() {
    return "Mapping " + pretty();
  }

  // virtual Formula* clone() {
  //   return m.get<Mapping>(ident_, mapping_id_, param_id_);
  // }

  virtual void TseitinTransformation(CnfFormula* cnf, bool top) {
    if (top)
      cnf->addClause({ getValue(*cnf->build_for_params()) });
  }
};

/******************************************************************************
 * Prepositional variable. The only possible leaf of the formula AST.
 */
class Variable: public Formula {
  std::string ident_;
  // bool orig_;
  // bool generated_;
  VarId id_;

  static VarId id_counter_; // initialy 1

 public:
  /* Parameterless constructor creates a generated variable with next
   * available id and named var_ID.
   */
  // Variable()
  //     : Formula(0),
  //       orig_(false),
  //       generated_(true) {
  //   id_ = id_counter_++;
  //   ident_ = "var" + std::to_string(id_);
  // }

  explicit Variable(std::string ident)
      : Formula(0),
        ident_(ident)
//        orig_(false),
//        generated_(false) {
  { }

  static VarId NewVarId() { return id_counter_++; }

  virtual uint node_id() { return kVariableId; }

  VarId id() { return id_; }
  void set_id(VarId value) {
    id_ = value;
    if (value > id_counter_) id_counter_ = value + 1;
  }

  // bool generated() { return generated_; }
  // bool orig() { return orig_; }
  // void set_orig(bool value) { orig_= value; }

  virtual std::string ident() { return ident_; }

  virtual std::string pretty(bool = true, std::vector<CharId>* = nullptr) {
    return ident_;
  }

  virtual std::string name() {
    return "Variable " + pretty() + "(" + std::to_string(id_) +")";
  }

  virtual Formula* child(uint) {
    // this have no children - child() should never be called
    assert(false);
  }

  virtual VarId tseitin_var(std::vector<CharId>* = nullptr) {
    return id_;
  }

  // virtual Formula* clone() {
    // if (variable_substitute_ && variable_substitute_->count(this) > 0) {
    //   return variable_substitute_->at(this);
    // } else if (index_substitute_) {
    //   std::vector<int> newIndices(indices_);
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

  virtual void TseitinTransformation(CnfFormula* cnf, bool top) {
    if (top)
      cnf->addClause({ id_ });
  }
};

#endif  // COBRA_FORMULA_H_

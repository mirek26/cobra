/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cassert>
#include <vector>
#include <map>
#include <string>
#include <initializer_list>
#include "util.h"
#include "parser.h"
#include "cnf-formula.h"

#ifndef COBRA_FORMULA_H_
#define COBRA_FORMULA_H_

class Formula;
class Variable;
class OrOperator;
class AndOperator;
class NotOperator;
class FormulaList;

/* Global Parser object. You should never create a ast node directly,
 * always use m.get<Type>(...).
 */
extern Parser m;

/* Prepositional formula, base class for
 * Derived classes:
 *  - NaryOperator - base class for associative n-ary operators
 *    - AndOperator - logical conjunction
 *    - OrOperator - logical disjunction
 *    - MacroOperator - syntax sugar - expansion can lead to exponential blowup
 *      - AtLeastOperator - at least k of n formulas must be true
 *      - AtMostOperator - at most k of n formulas must be true
 *      - ExactlyOperator - exactly k of n formulas must be true
 *  - EquivalenceOperator - logical equivalence (binary, symmetric)
 *  - ImpliesOperator - logical implication (binary)
 *  - NotOperator - negation of any other formula
 *  - Variable - the only possible leaf of the formula tree
 *  TODO: how about true/false leaves?
 */
class Formula: public Construct {
 protected:
  /* variable (possibly negated) that is equivalent to this subformula,
   * used for Tseitin transformation
   */
  Variable* tseitin_var_ = nullptr;

 public:
  /*
   */
  explicit Formula(int childCount)
      : Construct(childCount) { }

  /* Getter function for tseitin_var_. If tseitin_var_ was not used yet,
   * a new variable will be created.
   */
  virtual Formula* tseitin_var();

  /* Negate the formula. Equivalent to m.get<NotOperator>(this), except for
   * NotOperator nodes, for which it just returns the child (and thus avoids
   * having two NotOperator nodes under each other).
   */
  virtual Formula* neg();

  /* Dump this subtree in the same format as Construct::dump, but also
   * prints information about tseitin_var_.
   */
  virtual void dump(int indent = 0);

  /* Returns formula as a string. If utf8 is set to true, special math symbols
   * for conjunction, disjunction, implication etc. will be used.
   */
  virtual std::string pretty(bool utf8 = true) = 0;

  /* Returns true for variables or a negations of a variable, false otherwise.
   */
  virtual bool isLiteral() { return false; }

  /* Tseitin transformation - used for conversion to CNF.
   * Capures the relationsships between this node and its children as clauses
   * that are added to the vector 'clauses'. Recursively calls the same
   * on all children.
   */
  virtual void TseitinTransformation(CnfFormula* cnf, bool top) = 0;

  /* Converts the formula to CNF using Tseitin transformation.
   */
  CnfFormula* ToCnf();

  virtual Formula* clone() = 0;

  Formula* Substitude(std::map<Variable*, Variable*>& table);

  static Formula* Parse(std::string str);
};

/******************************************************************************
 * List of Formulas.
 */
class FormulaList: public VectorConstruct<Formula*> {
 private:
  void BuildRange(Formula* formula);

 public:
  FormulaList() { }

  virtual std::string name() {
    return "FormulaList";
  }

  FormulaList* clone() {
    auto n = m.get<FormulaList>();
    for (auto c: *this) {
      n->push_back(c->clone());
    }
    return n;
  }

  static FormulaList* Range(Formula* formula) {
    auto result = m.get<FormulaList>();
    result->BuildRange(formula);
    m.formulaListRange().clear();
    return result;
  }
};

/******************************************************************************
 * Base class for associative n-ary operator. Abstract.
 */
class NaryOperator: public Formula {
 protected:
  FormulaList* children_;

 public:
  NaryOperator()
      : Formula(1) {
    children_ = m.get<FormulaList>();
  }

  explicit NaryOperator(std::initializer_list<Formula*> list)
      : Formula(1) {
    children_ = m.get<FormulaList>();
    children_->insert(children_->begin(), list.begin(), list.end());
  }

  explicit NaryOperator(FormulaList* list)
      : Formula(1),
        children_(list) {
    assert(list);
  }

  void addChild(Formula* child) {
    children_->push_back(child);
  }

  void addChildren(FormulaList* children) {
    children_->insert(children_->end(), children->begin(), children->end());
  }

  void removeChild(int nth) {
    (*children_)[nth] = children_->back();
    children_->pop_back();
  }

  virtual Construct* child(uint nth) {
    assert(nth == 0);
    return children_;
  }

  FormulaList* children() {
    return children_;
  }

 protected:
  std::string pretty_join(std::string sep, bool utf8) {
    if (children_->empty()) return "()";
    std::string s = "(" + children_->front()->pretty(utf8);
    for (auto it = std::next(children_->begin()); it != children_->end(); ++it) {
      s += sep;
      s += (*it)->pretty(utf8);
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
  explicit AndOperator(FormulaList* list)
      : NaryOperator(list) { }

  virtual std::string name() {
    return "AndOperator";
  }

  virtual std::string pretty(bool utf8 = true) {
    return pretty_join(utf8 ? " ∧ " : " & ", utf8);
  }

  virtual Formula* clone() {
    return m.get<AndOperator>(children_->clone());
  }

  virtual Construct* Simplify();
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
  explicit OrOperator(FormulaList* list)
      : NaryOperator(list) {}

  virtual std::string name() {
    return "OrOperator";
  }

  virtual std::string pretty(bool utf8 = true) {
    return pretty_join(utf8 ? " ∨ " : " | ", utf8);
  }

  virtual Formula* clone() {
    return m.get<OrOperator>(children_->clone());
  }

  virtual Construct* Simplify();
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
  explicit MacroOperator(FormulaList* list);

  virtual Formula* tseitin_var();

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
  AtLeastOperator(uint value, FormulaList* list)
      : MacroOperator(list),
        value_(value) {
    assert(value <= list->size());
  }

  virtual std::string name() {
    return "AtLeastOperator(" + std::to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true) {
    return "AtLeast-" + std::to_string(value_) + pretty_join(", ", utf8);
  }

  virtual Formula* clone() {
    return m.get<AtLeastOperator>(value_, children_->clone());
  }

  virtual AndOperator* Expand();
};

/******************************************************************************
 *
 */
class AtMostOperator: public MacroOperator {
  uint value_;

 public:
  AtMostOperator(uint value, FormulaList* list)
      : MacroOperator(list),
        value_(value) {
    assert(value <= list->size());
  }

  virtual std::string name() {
    return "AtMostOperator(" + std::to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true) {
    return "AtMost-" + std::to_string(value_) + pretty_join(", ", utf8);
  }

  virtual Formula* clone() {
    return m.get<AtMostOperator>(value_, children_->clone());
  }

  virtual AndOperator* Expand();
};

/******************************************************************************
 *
 */
class ExactlyOperator: public MacroOperator {
  uint value_;

 public:
  ExactlyOperator(uint value, FormulaList* list)
      : MacroOperator(list),
        value_(value) {
    assert(value <= list->size());
  }

  virtual std::string name() {
    return "ExactlyOperator(" + std::to_string(value_) + ")";
  }

  virtual std::string pretty(bool utf8 = true) {
    return "Exactly-" + std::to_string(value_) + pretty_join(", ", utf8);
  }

  virtual Formula* clone() {
    return m.get<ExactlyOperator>(value_, children_->clone());
  }

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

  virtual std::string name() {
    return "EquivalenceOperator";
  }

  virtual std::string pretty(bool utf8 = true) {
    return "(" +
      left_->pretty(utf8) +
      (utf8 ? " ⇔ " : " <-> ") +
      right_->pretty(utf8) +
      ")";
  }

  virtual Construct* child(uint nth) {
    assert(nth < child_count());
    switch (nth) {
      case 0: return left_;
      case 1: return right_;
      default: assert(false);
    }
  }

  virtual Formula* clone() {
    return m.get<EquivalenceOperator>(left_->clone(), right_->clone());
  }

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

  virtual std::string name() {
    return "ImpliesOperator";
  }

  virtual std::string pretty(bool utf8 = true) {
    return "(" +
      left_->pretty(utf8) +
      (utf8 ? " ⇒ " : " -> ") +
      right_->pretty(utf8) +
      ")";
  }

  virtual Construct* child(uint nth) {
    assert(nth < child_count());
    switch (nth) {
      case 0: return left_;
      case 1: return right_;
      default: assert(false);
    }
  }

  virtual Formula* clone() {
    return m.get<ImpliesOperator>(left_->clone(), right_->clone());
  }

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

  virtual std::string name() {
    return "NotOperator";
  }

  virtual std::string pretty(bool utf8 = true) {
    return (utf8 ? "¬" : "!") + child_->pretty(utf8);
  }

  virtual Construct* child(uint nth) {
    assert(nth == 0);
    return child_;
  }

  virtual Formula* clone() {
    return m.get<NotOperator>(child_->clone());
  }

  virtual Formula* neg() {
    return child_;
  }

  virtual bool isLiteral() {
    return child_->isLiteral();
  }

  virtual void TseitinTransformation(CnfFormula* cnf, bool top);
};

/******************************************************************************
 * Prepositional variable. The only possible leaf of the formula AST.
 */
class Variable: public Formula {
  std::string ident_;
  std::vector<int> indices_;
  bool orig_;
  bool generated_;
  int id_;

  static int id_counter_; // initialy 1

 public:
  static std::map<Variable*, Variable*>* variable_substitute_; // = nullptr
  static std::map<int, int>* index_substitute_; // = nullptr

 public:
  /* Parameterless constructor creates a generated variable with next
   * available id and named var_ID.
   */
  Variable()
      : Formula(0),
        orig_(false),
        generated_(true) {
    id_ = id_counter_++;
    ident_ = "var" + std::to_string(id_);
  }

  explicit Variable(std::string ident)
      : Formula(0),
        ident_(ident),
        orig_(false),
        generated_(false) {
    id_ = id_counter_++;
  }

  Variable(std::string ident, const std::vector<int>& indices)
      : Formula(0),
        ident_(ident),
        orig_(false),
        generated_(false) {
    indices_.insert(indices_.begin(), indices.begin(), indices.end());
    id_ = id_counter_++;
  }

  int id() {
    return id_;
  }

  const std::vector<int>& indices() {
    return indices_;
  }

  bool generated() {
    return generated_;
  }

  bool orig() {
    return orig_;
  }

  void orig(bool value) {
    orig_= value;
  }

  virtual std::string ident() {
    return ident_;
  }

  virtual std::string pretty(bool = true) {
    return ident_ + joinIndices(indices_);
  }

  virtual std::string name() {
    return "Variable " + pretty() + "(" + std::to_string(id_) +")";
  }

  virtual Construct* child(uint) {
    // this have no children - child() should never be called
    assert(false);
  }

  bool isParam() {
    return ident_ == "p" && indices_.size() == 1;
  }

  uint getParam() {
    return isParam() ? indices_[0] : 0;
  }

  virtual Formula* clone() {
    if (variable_substitute_ && variable_substitute_->count(this) > 0) {
      return variable_substitute_->at(this);
    } else if (index_substitute_) {
      std::vector<int> newIndices(indices_);
      for (auto& i: newIndices) {
        if (index_substitute_->count(i) > 0) {
          i = index_substitute_->at(i);
        }
      }
      return m.get<Variable>(ident_, newIndices);
    } else {
      return this;
    }
  }

  virtual bool isLiteral() {
    return true;
  }

  virtual void TseitinTransformation(CnfFormula* cnf, bool top) {
    if (top) {
      cnf->addClause({ (Formula*)this });
    }
  }

  static std::string joinIndices(const std::vector<int> list) {
    if (list.empty()) return "";
    std::string s = std::to_string(list.front());
    for (auto it = std::next(list.begin()); it != list.end(); ++it) {
      s += "_" + std::to_string(*it);
    }
    return s;
  }
};

/******************************************************************************
 * Set of Variables.
 */
class VariableSet: public VectorConstruct<Variable*> {
 public:
  VariableSet() { }

  virtual std::string name() {
    return "VariableSet";
  }

  void sort() {
    std::sort(begin(), end(), [](Variable* a, Variable* b){
      return b->id() > a->id();
    });
  }

  FormulaList* asFormulaList() {
    auto v = m.get<FormulaList>();
    v->insert(v->begin(), this->begin(), this->end());
    return v;
  }

  static VariableSet* Range(Variable* from, Variable* to) {
    auto vars = m.get<VariableSet>();
    if (from->ident() != to->ident()) {
      throw new ParserException("Invalid range.");
    }
    std::vector<int> v;
    v.insert(v.begin(), from->indices().begin(), from->indices().end());
    assert(from->indices().size() >= 1);
    assert(to->indices().size() >= 1);
    for (int i = from->indices()[0]; i <= to->indices()[0]; i++) {
      v[0] = i;
      vars->push_back(m.get<Variable>(from->ident(), v));
    }
    return vars;
  }
};


class ParameterEq: public Formula {
  int param_;
  Variable* var_;

 public:
  ParameterEq(Variable* param, Variable* var)
      : Formula(1),
        var_(var) {
    assert(param->ident() == "p");
    assert(param->indices().size() == 1);
    param_ = param->indices()[0];
    // if absolute position, decrease by one (p1 is at parameter at position 0)
    if (param_ > 0) param_--;
  }

  ParameterEq(int param, Variable* var)
      : Formula(1),
        param_(param),
        var_(var) {}

  uint tseitin_id() {
    assert(tseitin_var_);
    return tseitin_var_->id();
  }

  uint param() {
    return param_;
  }

  Variable* var() {
    return var_;
  }

  virtual std::string name() {
    return "ParameterEq";
  }

  virtual std::string pretty(bool = true) {
    return "(p" + std::to_string(param_ + 1) + " == " + var_->pretty() + ")";
  }

  virtual Construct* child(uint nth) {
    assert(nth == 0);
    return var_;
  }

  virtual void TseitinTransformation(CnfFormula* cnf, bool) {
    cnf->addParameterEq(this);
  }

  virtual Formula* clone() {
    return m.get<ParameterEq>(
      (Variable::index_substitute_ && Variable::index_substitute_->count(param_) ?
        Variable::index_substitute_->at(param_)-1 : param_),
      static_cast<Variable*>(var_->clone()));
  }
};

#endif  // COBRA_FORMULA_H_

/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
#include <map>

#include "formula.h"
#include "util.h"

int Variable::id_counter_ = 1;
std::map<Variable*, Variable*>* Formula::variable_substitude_ = nullptr;

extern void parse_string(std::string s);

Formula* Formula::Parse(std::string str) {
  parse_string(str);
  assert(m.onlyFormula());
  return m.onlyFormula();
}

Formula* Formula::Substitude(std::map<Variable*, Variable*>& table) {
  variable_substitude_ = &table;
  auto n = this->clone();
  variable_substitude_ = nullptr;
  return n;
}

void Formula::dump(int indent) {
  for (int i = 0; i < indent; ++i) {
    printf("   ");
  }
  std::string var = tseitin_var_ ? tseitin_var_->pretty() :
                    isLiteral() ? pretty() : "-";
  printf("%p: %s (%s)\n", (void*)this, name().c_str(), var.c_str());
  for (uint i = 0; i < child_count(); ++i) {
    child(i)->dump(indent + 1);
  }
}

Formula* Formula::neg() {
  return m.get<NotOperator>(this);
}

Construct* AndOperator::Simplify() {
  Construct::Simplify();
  int merged = 0;
  int childCount = child_count();
  for (int i = 0; i < childCount; ++i) {
    auto andChild = dynamic_cast<AndOperator*>(child(i - merged));
    if (andChild) {
      removeChild(i - merged);
      addChildren(andChild->children_);
      merged++;
    }
  }
  return this;
}

Construct* OrOperator::Simplify() {
  Construct::Simplify();
  int merged = 0;
  int childCount = child_count();
  for (int i = 0; i < childCount; ++i) {
    auto orChild = dynamic_cast<OrOperator*>(child(i - merged));
    if (orChild) {
      removeChild(i - merged);
      addChildren(orChild->children_);
      merged++;
    }
  }
  return this;
}

// Tseitin transformation of arbitrary formula to CNF
AndOperator* Formula::ToCnf() {
  FormulaList* clauses = m.get<FormulaList>();
  // recursively create clauses capturing relationships between nodes
  TseitinTransformation(clauses, true);
  auto root = m.get<AndOperator>(clauses);
  return root;
}

// return the variable corresponding to the node during Tseitin transformation
Formula* Formula::tseitin_var() {
  if (isLiteral()) return this;
  if (!tseitin_var_) tseitin_var_ = m.get<Variable>();
  return tseitin_var_;
}

/******************************************************************************
 * Expansion of macros.
 */

Formula* MacroOperator::tseitin_var() {
  /* Macros must be expanded before tseitin tranformation. Never create a
   * tseitin_var_, return a tseitin_var_ of the root of the expanded subtree.
   */
  return expanded_->tseitin_var();
}

MacroOperator::MacroOperator(FormulaList* list)
      : NaryOperator(list) {
  expanded_ = m.get<AndOperator>();
}

void MacroOperator::ExpandHelper(uint size, bool negate) {
  assert(size <= children_->size());
  std::vector<Formula*> vars;
  for (auto& f: *children_) {
    vars.push_back(f->tseitin_var());
  }
  //
  AndOperator* root = expanded_;
  std::function<void(std::vector<Formula*>)> add = [&](std::vector<Formula*> list) {
    auto clause = m.get<OrOperator>();
    for (auto& f: list) clause->addChild(negate ? f->neg() : f);
    root->addChild(clause);
  };
  //
  for_all_combinations(size, vars, add);
}

AndOperator* AtLeastOperator::Expand() {
  if (value_ > 0) {
    ExpandHelper(children_->size() - value_ + 1, false);
  }
  return expanded_;
}

AndOperator* AtMostOperator::Expand() {
  if (value_ < children_->size()) {
    ExpandHelper(value_ + 1, true);
  }
  return expanded_;
}

AndOperator* ExactlyOperator::Expand() {
  /* TODO: it might be better to have expanded_ as an OrOperator and just
   * list all the possible combinations here.
   */
  // At most part
  if (value_ < children_->size()) {
    ExpandHelper(value_ + 1, true);
  }
  // At least part
  if (value_ > 0) {
    ExpandHelper(children_->size() - value_ + 1, false);
  }
  return expanded_;
}

/******************************************************************************
 * Tseitin transformation.
 */

void AndOperator::TseitinTransformation(FormulaList* clauses, bool top) {
  // if tseitin_var_ was not passigned yet, just recurse down
  if (!top) {
    // X <-> AND(A1, A2, ..)
    // (X | !A1 | !A2 | ...) & (A1 | !X) & (A2 | !X) & ...
    auto first = m.get<FormulaList>();
    for (auto& f: *children_) {
      first->push_back(f->tseitin_var()->neg());
    }
    first->push_back(tseitin_var());
    clauses->push_back(m.get<OrOperator>(first));

    auto not_this = tseitin_var()->neg();
    for (auto& f: *children_) {
      clauses->push_back(m.get<OrOperator>({ not_this, f->tseitin_var() }));
    }
  }
  // recurse down
  for (auto& f: *children_) {
    f->TseitinTransformation(clauses, top);
  }
}

void OrOperator::TseitinTransformation(FormulaList* clauses, bool top) {
  auto first = m.get<FormulaList>();
  for (auto& f: *children_) {
    first->push_back(f->tseitin_var());
  }

  if (!top) {
    // X <-> OR(A1, A2, ..)
    // (!X | A1 | A2 | ...) & (!A1 | X) & (!A2 | X) & ...
    first->push_back(tseitin_var()->neg());
    clauses->push_back(m.get<OrOperator>(first));

    for (auto& f: *children_) {
      clauses->push_back(
        m.get<OrOperator>({ tseitin_var(), f->tseitin_var()->neg() }));
    }
  } else {
    clauses->push_back(m.get<OrOperator>(first));
  }
  // recurse down
  for (auto&f : *children_) {
    f->TseitinTransformation(clauses, false);
  }
}

void NotOperator::TseitinTransformation(FormulaList* clauses, bool top) {
  // X <-> (!Y)
  // (!X | !Y) & (X | Y)
  auto thisVar = tseitin_var();
  auto childVar = child_->tseitin_var();
  if (top) clauses->push_back(m.get<OrOperator>({ tseitin_var() }));
  if (!this->isLiteral()) {
    clauses->push_back(m.get<OrOperator>({ thisVar->neg(), childVar->neg() }));
    clauses->push_back(m.get<OrOperator>({ thisVar, childVar }));
    child_->TseitinTransformation(clauses, false);
  }
}

void ImpliesOperator::TseitinTransformation(FormulaList* clauses, bool top) {
  auto thisVar = tseitin_var();
  auto leftVar = left_->tseitin_var();
  auto rightVar = right_->tseitin_var();
  if (top) {
    clauses->push_back(m.get<OrOperator>({ leftVar->neg(), rightVar }));
  } else {
    // X <-> (L -> R)
    // (!X | !L | R) & (L | X) & (!R | X)
    clauses->push_back(
      m.get<OrOperator>({ thisVar->neg(), leftVar->neg(), rightVar }));
    clauses->push_back(
      m.get<OrOperator>({ leftVar, thisVar }));
    clauses->push_back(
      m.get<OrOperator>({ rightVar->neg(), thisVar }));
  }
  left_->TseitinTransformation(clauses, false);
  right_->TseitinTransformation(clauses, false);
}

void EquivalenceOperator::TseitinTransformation(FormulaList* clauses, bool top) {
  auto thisVar = tseitin_var();
  auto leftVar = left_->tseitin_var();
  auto rightVar = right_->tseitin_var();
  if (top) {
    clauses->push_back(m.get<OrOperator>({ leftVar->neg(), rightVar }));
    clauses->push_back(m.get<OrOperator>({ leftVar, rightVar->neg() }));
  } else {
    // X <-> (L <-> R)
    // (X | L | R) & (!X | !L | R) & (!X | L | !R) & (X | !L | !R)
    clauses->push_back(
      m.get<OrOperator>({ thisVar, leftVar, rightVar }));
    clauses->push_back(
      m.get<OrOperator>({ thisVar->neg(), leftVar->neg(), rightVar }));
    clauses->push_back(
      m.get<OrOperator>({ thisVar->neg(), leftVar, rightVar->neg() }));
    clauses->push_back(
      m.get<OrOperator>({ thisVar, leftVar->neg(), rightVar->neg() }));
  }
  left_->TseitinTransformation(clauses, false);
  right_->TseitinTransformation(clauses, false);
}

void MacroOperator::TseitinTransformation(FormulaList* clauses, bool top) {
  auto expanded = Expand();
  expanded->TseitinTransformation(clauses, top);
}

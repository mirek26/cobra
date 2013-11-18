/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <algorithm>
#include <string>
#include "formula.h"
#include "util.h"

int Variable::id_counter_ = 1;

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
  TseitinTransformation(clauses);
  // the variable corresponding to the root node must be always true
  clauses->push_back(m.get<OrOperator>({ tseitin_var() }));
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

void MacroOperator::ExpandHelper(int size, bool negate) {
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
  ExpandHelper(children_->size() - value_ + 1, false);
  return expanded_;
}

AndOperator* AtMostOperator::Expand() {
  ExpandHelper(value_ + 1, true);
  return expanded_;
}

AndOperator* ExactlyOperator::Expand() {
  /* TODO: it might be better to have expanded_ as an OrOperator and just
   * list all the possible combinations here.
   */
  // At most part
  ExpandHelper(value_ + 1, true);
  // At least part
  ExpandHelper(children_->size() - value_ + 1, false);
  return expanded_;
}

/******************************************************************************
 * Tseitin transformation.
 */

void AndOperator::TseitinTransformation(FormulaList* clauses) {
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

  // recurse down
  for (auto& f: *children_) {
    f->TseitinTransformation(clauses);
  }
}

void OrOperator::TseitinTransformation(FormulaList* clauses) {
  // X <-> OR(A1, A2, ..)
  // (!X | A1 | A2 | ...) & (!A1 | X) & (!A2 | X) & ...
  auto first = m.get<FormulaList>();
  for (auto& f: *children_) {
    first->push_back(f->tseitin_var());
  }
  first->push_back(tseitin_var()->neg());
  clauses->push_back(m.get<OrOperator>(first));

  for (auto& f: *children_) {
    clauses->push_back(
      m.get<OrOperator>({ tseitin_var(), f->tseitin_var()->neg() }));
  }

  // recurse down
  for (auto&f : *children_) {
    f->TseitinTransformation(clauses);
  }
}

void NotOperator::TseitinTransformation(FormulaList* clauses) {
  // X <-> (!Y)
  // (!X | !Y) & (X | Y)
  auto thisVar = tseitin_var();
  auto childVar = child_->tseitin_var();
  clauses->push_back(m.get<OrOperator>({ thisVar->neg(), childVar->neg() }));
  clauses->push_back(m.get<OrOperator>({ thisVar, childVar }));
  child_->TseitinTransformation(clauses);
}

void ImpliesOperator::TseitinTransformation(FormulaList* clauses) {
  // X <-> (L -> R)
  // (!X | !L | R) & (L | X) & (!R | X)
  auto thisVar = tseitin_var();
  auto leftVar = left_->tseitin_var();
  auto rightVar = right_->tseitin_var();
  clauses->push_back(
    m.get<OrOperator>({ thisVar->neg(), leftVar->neg(), rightVar }));
  clauses->push_back(
    m.get<OrOperator>({ leftVar, thisVar }));
  clauses->push_back(
    m.get<OrOperator>({ rightVar->neg(), thisVar }));

  left_->TseitinTransformation(clauses);
  right_->TseitinTransformation(clauses);
}

void EquivalenceOperator::TseitinTransformation(FormulaList* clauses) {
  // X <-> (L <-> R)
  // (X | L | R) & (!X | !L | R) & (!X | L | !R) & (X | !L | !R)
  auto thisVar = tseitin_var();
  auto leftVar = left_->tseitin_var();
  auto rightVar = right_->tseitin_var();
  clauses->push_back(
    m.get<OrOperator>({ thisVar, leftVar, rightVar }));
  clauses->push_back(
    m.get<OrOperator>({ thisVar->neg(), leftVar->neg(), rightVar }));
  clauses->push_back(
    m.get<OrOperator>({ thisVar->neg(), leftVar, rightVar->neg() }));
  clauses->push_back(
    m.get<OrOperator>({ thisVar, leftVar->neg(), rightVar->neg() }));

  left_->TseitinTransformation(clauses);
  right_->TseitinTransformation(clauses);
}

void MacroOperator::TseitinTransformation(FormulaList* clauses) {
  auto expanded = Expand();
  expanded->TseitinTransformation(clauses);
}

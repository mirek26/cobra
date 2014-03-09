/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
#include <map>

#include "formula.h"
#include "common.h"

VarId Variable::id_counter_ = 1;

extern void parse_string(std::string s);

Formula* Formula::Parse(std::string str) {
  parse_string(str);
  assert(m.only_formula());
  return m.only_formula();
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

Formula* AndOperator::Simplify() {
  Formula::Simplify();
  int merged = 0;
  int childCount = child_count();
  for (int i = 0; i < childCount; ++i) {
    auto andChild = dynamic_cast<AndOperator*>(child(i - merged));
    if (andChild) {
      removeChild(i - merged);
      children_.insert(children_.end(), andChild->children_.begin(),
                                        andChild->children_.end());
      merged++;
    }
  }
  return this;
}

Formula* OrOperator::Simplify() {
  Formula::Simplify();
  int merged = 0;
  int childCount = child_count();
  for (int i = 0; i < childCount; ++i) {
    auto orChild = dynamic_cast<OrOperator*>(child(i - merged));
    if (orChild) {
      removeChild(i - merged);
      children_.insert(children_.end(), orChild->children_.begin(),
                                        orChild->children_.end());
      merged++;
    }
  }
  return this;
}

// Tseitin transformation of arbitrary formula to CNF
CnfFormula* Formula::ToCnf() {
  CnfFormula* r = new CnfFormula();
  TseitinTransformation(r, true);
  return r;
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

MacroOperator::MacroOperator(std::vector<Formula*>* list)
      : NaryOperator(list) {
  expanded_ = m.get<AndOperator>();
}

void MacroOperator::ExpandHelper(uint size, bool negate) {
  assert(size <= children_.size());
  std::vector<Formula*> vars;
  for (auto& f: children_) {
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
    ExpandHelper(children_.size() - value_ + 1, false);
  }
  return expanded_;
}

AndOperator* AtMostOperator::Expand() {
  if (value_ < children_.size()) {
    ExpandHelper(value_ + 1, true);
  }
  return expanded_;
}

AndOperator* ExactlyOperator::Expand() {
  /* TODO: it might be better to have expanded_ as an OrOperator and just
   * list all the possible combinations here.
   */
  // At most part
  if (value_ < children_.size()) {
    ExpandHelper(value_ + 1, true);
  }
  // At least part
  if (value_ > 0) {
    ExpandHelper(children_.size() - value_ + 1, false);
  }
  return expanded_;
}

/******************************************************************************
 * Tseitin transformation.
 */

void AndOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  // if tseitin_var_ was not passigned yet, just recurse down
  if (!top) {
    // X <-> AND(A1, A2, ..)
    // (X | !A1 | !A2 | ...) & (A1 | !X) & (A2 | !X) & ...
    auto first = new std::vector<Formula*>();
    for (auto& f: children_) {
      first->push_back(f->tseitin_var()->neg());
    }
    first->push_back(tseitin_var());
    cnf->addClause(first);
    auto not_this = tseitin_var()->neg();
    for (auto& f: children_) {
      cnf->addClause({ not_this, f->tseitin_var() });
    }
  }
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, top);
  }
}

void OrOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto first = new std::vector<Formula*>();
  for (auto& f: children_) {
    first->push_back(f->tseitin_var());
  }

  if (!top) {
    // X <-> OR(A1, A2, ..)
    // (!X | A1 | A2 | ...) & (!A1 | X) & (!A2 | X) & ...
    first->push_back(tseitin_var()->neg());
    cnf->addClause(first);
    for (auto& f: children_) {
      cnf->addClause({ tseitin_var(), f->tseitin_var()->neg() });
    }
  } else {
    cnf->addClause(first);
  }
  // recurse down
  for (auto&f : children_) {
    f->TseitinTransformation(cnf, false);
  }
}

void NotOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  // X <-> (!Y)
  // (!X | !Y) & (X | Y)
  auto thisVar = tseitin_var();
  auto childVar = child_->tseitin_var();
  if (top) cnf->addClause({ tseitin_var() });
  if (!this->isLiteral()) {
    cnf->addClause({ thisVar->neg(), childVar->neg() });
    cnf->addClause({ thisVar, childVar });
    child_->TseitinTransformation(cnf, false);
  }
}

void ImpliesOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var();
  auto leftVar = left_->tseitin_var();
  auto rightVar = right_->tseitin_var();
  if (top) {
    cnf->addClause({ leftVar->neg(), rightVar });
  } else {
    // X <-> (L -> R)
    // (!X | !L | R) & (L | X) & (!R | X)
    cnf->addClause({ thisVar->neg(), leftVar->neg(), rightVar });
    cnf->addClause({ leftVar, thisVar });
    cnf->addClause({ rightVar->neg(), thisVar });
  }
  left_->TseitinTransformation(cnf, false);
  right_->TseitinTransformation(cnf, false);
}

void EquivalenceOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var();
  auto leftVar = left_->tseitin_var();
  auto rightVar = right_->tseitin_var();
  if (top) {
    cnf->addClause({ leftVar->neg(), rightVar });
    cnf->addClause({ leftVar, rightVar->neg() });
  } else {
    // X <-> (L <-> R)
    // (X | L | R) & (!X | !L | R) & (!X | L | !R) & (X | !L | !R)
    cnf->addClause({ thisVar, leftVar, rightVar });
    cnf->addClause({ thisVar->neg(), leftVar->neg(), rightVar });
    cnf->addClause({ thisVar->neg(), leftVar, rightVar->neg() });
    cnf->addClause({ thisVar, leftVar->neg(), rightVar->neg() });
  }
  left_->TseitinTransformation(cnf, false);
  right_->TseitinTransformation(cnf, false);
}

void MacroOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto expanded = Expand();
  expanded->TseitinTransformation(cnf, top);
}

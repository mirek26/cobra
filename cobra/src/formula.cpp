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
  printf("%p: %s\n", (void*)this, name().c_str());
  for (uint i = 0; i < child_count(); ++i) {
    child(i)->dump(indent + 1);
  }
}

Formula* Formula::neg() {
  return m.get<NotOperator>(this);
}

std::string Mapping::pretty(bool, std::vector<CharId>* params) {
  if (params) {
    return m.game().variables()[getValue(*params)]->pretty();
  } else {
    return ident_ + "$" + std::to_string(param_id_);
  }
}

// Tseitin transformation of arbitrary formula to CNF
CnfFormula* Formula::ToCnf() {
  CnfFormula* r = new CnfFormula();
  TseitinTransformation(r, true);
  return r;
}

CnfFormula* Formula::ToCnf(std::vector<CharId>& param) {
  CnfFormula* r = new CnfFormula();
  r->set_build_for_params(&param);
  TseitinTransformation(r, true);
  r->set_build_for_params(nullptr);
  return r;
}

// return the variable corresponding to the node during Tseitin transformation
VarId Formula::tseitin_var(std::vector<CharId>*) {
  if (tseitin_var_ == 0) tseitin_var_ = Variable::NewVarId();
  return tseitin_var_;
}

/******************************************************************************
 * Expansion of macros.
 */

// VarId MacroOperator::tseitin_var() {
//    Macros must be expanded before tseitin tranformation. Never create a
//    * tseitin_var_, return a tseitin_var_ of the root of the expanded subtree.

//   return expanded_->tseitin_var();
// }

MacroOperator::MacroOperator(std::vector<Formula*>* list)
      : NaryOperator(list) {
  //expanded_ = m.get<AndOperator>();
}

void MacroOperator::ExpandHelper(uint size, bool negate) {
  // assert(size <= children_.size());
  // std::vector<Formula*> vars;
  // for (auto& f: children_) {
  //   vars.push_back(f->tseitin_var());
  // }
  // //
  // AndOperator* root = expanded_;
  // std::function<void(std::vector<Formula*>)> add = [&](std::vector<VarId> list) {
  //   auto clause = m.get<OrOperator>();
  //   for (auto& f: list) clause->addChild(negate ? -f : f);
  //   root->addChild(clause);
  // };
  // //
  // for_all_combinations(size, vars, add);
}

AndOperator* AtLeastOperator::Expand() {
  // if (value_ > 0) {
  //   ExpandHelper(children_.size() - value_ + 1, false);
  // }
  // return expanded_;
  return nullptr;
}

AndOperator* AtMostOperator::Expand() {
  // if (value_ < children_.size()) {
  //   ExpandHelper(value_ + 1, true);
  // }
  // return expanded_;
  return nullptr;
}

AndOperator* ExactlyOperator::Expand() {
  /* TODO: it might be better to have expanded_ as an OrOperator and just
   * list all the possible combinations here.
   */
  // // At most part
  // if (value_ < children_.size()) {
  //   ExpandHelper(value_ + 1, true);
  // }
  // // At least part
  // if (value_ > 0) {
  //   ExpandHelper(children_.size() - value_ + 1, false);
  // }
  // return expanded_;
  return nullptr;
}

/******************************************************************************
 * Tseitin transformation.
 */

void AndOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  // if tseitin_var_ was not passigned yet, just recurse down
  if (!top) {
    // X <-> AND(A1, A2, ..)
    // (X | !A1 | !A2 | ...) & (A1 | !X) & (A2 | !X) & ...
    auto thisVar = tseitin_var(cnf->build_for_params());
    std::vector<VarId> first;
    for (auto& f: children_) {
      first.push_back(-f->tseitin_var(cnf->build_for_params()));
    }
    first.push_back(tseitin_var());
    cnf->addClause(first);
    for (auto& f: children_) {
      cnf->addClause({ -thisVar, f->tseitin_var(cnf->build_for_params()) });
    }
  }
  // recurse down
  for (auto& f: children_) {
    f->TseitinTransformation(cnf, top);
  }
}

void OrOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  std::vector<VarId> first;
  for (auto& f: children_) {
    first.push_back(f->tseitin_var(cnf->build_for_params()));
  }

  if (!top) {
    // X <-> OR(A1, A2, ..)
    // (!X | A1 | A2 | ...) & (!A1 | X) & (!A2 | X) & ...
    auto thisVar = tseitin_var(cnf->build_for_params());
    first.push_back(-thisVar);
    cnf->addClause(first);
    for (auto& f: children_) {
      cnf->addClause({ thisVar, -f->tseitin_var(cnf->build_for_params()) });
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
  auto thisVar = tseitin_var(cnf->build_for_params());
  auto childVar = child_->tseitin_var(cnf->build_for_params());
  if (top) cnf->addClause({ thisVar });
  if (!this->isLiteral()) {
    cnf->addClause({ -thisVar, -childVar });
    cnf->addClause({ thisVar, childVar });
    child_->TseitinTransformation(cnf, false);
  }
}

void ImpliesOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var(cnf->build_for_params());
  auto leftVar = left_->tseitin_var(cnf->build_for_params());
  auto rightVar = right_->tseitin_var(cnf->build_for_params());
  if (top) {
    cnf->addClause({ -leftVar, rightVar });
  } else {
    // X <-> (L -> R)
    // (!X | !L | R) & (L | X) & (!R | X)
    cnf->addClause({ -thisVar, -leftVar, rightVar });
    cnf->addClause({ leftVar, thisVar });
    cnf->addClause({ -rightVar, thisVar });
  }
  left_->TseitinTransformation(cnf, false);
  right_->TseitinTransformation(cnf, false);
}

void EquivalenceOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  auto thisVar = tseitin_var(cnf->build_for_params());
  auto leftVar = left_->tseitin_var(cnf->build_for_params());
  auto rightVar = right_->tseitin_var(cnf->build_for_params());
  if (top) {
    cnf->addClause({ -leftVar, rightVar });
    cnf->addClause({ leftVar, -rightVar });
  } else {
    // X <-> (L <-> R)
    // (X | L | R) & (!X | !L | R) & (!X | L | !R) & (X | !L | !R)
    cnf->addClause({ thisVar, leftVar, rightVar });
    cnf->addClause({ -thisVar, -leftVar, rightVar });
    cnf->addClause({ -thisVar, leftVar, -rightVar });
    cnf->addClause({ thisVar, -leftVar, -rightVar });
  }
  left_->TseitinTransformation(cnf, false);
  right_->TseitinTransformation(cnf, false);
}

void MacroOperator::TseitinTransformation(CnfFormula* cnf, bool top) {
  //auto expanded = Expand();
  //expanded->TseitinTransformation(cnf, top);
}

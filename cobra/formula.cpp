/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <algorithm>
#include <string>

#include "./formula.h"
#include "./util.h"

Variable* get_temp() {
  static int k = 0;
  return new Variable("t" + to_string(k++));
}

void Construct::dump(int indent) {
  for (int i = 0; i < indent; ++i) {
    printf("   ");
  }
  printf("%p: %s\n", (void*)this, getName().c_str());
  for (uint i = 0; i < getChildCount(); ++i) {
    getChild(i)->dump(indent + 1);
  }
}

Construct* AndOperator::optimize() {
  Construct::optimize();
  int merged = 0;
  int childCount = getChildCount();
  for (int i = 0; i < childCount; ++i) {
    auto child = getChild(i - merged);
    auto andChild = dynamic_cast<AndOperator*>(child);
    if (andChild) {
      removeChild(i - merged);
      addChilds(andChild->m_list);
      merged++;
      andChild->m_list.clear();
      delete andChild;
    }
  }
  return this;
}

Construct* OrOperator::optimize() {
  Construct::optimize();
  int merged = 0;
  int childCount = getChildCount();
  for (int i = 0; i < childCount; ++i) {
    auto child = getChild(i - merged);
    auto orChild = dynamic_cast<OrOperator*>(child);
    if (orChild) {
      removeChild(i - merged);
      addChilds(orChild->m_list);
      merged++;
      orChild->m_list.clear();
      delete orChild;
    }
  }
  return this;
}

AndOperator* Formula::to_cnf() {
  FormulaVec clauses;
  tseitin(clauses);
  clauses.push_back(new OrOperator(FormulaVec{ get_var() }));
  auto root = new AndOperator(clauses);
  return root;
}

Formula* Formula::get_var() {
  if (is_simple()) {
    return this;
  }
  if (!var) {
    var = get_temp();
  }
  return var;
}

AndOperator* AtLeastOperator::expand() {
  FormulaVec vars;
  std::function<Formula*(Formula*)> get_vars = [](Formula* f){ return f->get_var(); };
  transform(m_list, vars, get_vars);
  auto root = new AndOperator();
  std::function<void(FormulaVec)> addToNode =
    [&](FormulaVec l){
    root->addChild(new OrOperator(l));
  };
  for_all_combinations(vars.size() - m_value + 1, vars, addToNode);
  return root;
}

AndOperator* AtMostOperator::expand() {
  FormulaVec vars;
  std::function<Formula*(Formula*)> get_vars = [](Formula* f){ return f->get_var(); };
  transform(m_list, vars, get_vars);
  auto root = new AndOperator();
  std::function<void(FormulaVec)> addToNode =
    [&](FormulaVec l){
    FormulaVec negl;
    negl.resize(l.size());
    std::transform(l.begin(), l.end(), negl.begin(), [](Formula* f){ return new NotOperator(f); });
    root->addChild(new OrOperator(negl));
  };
  for_all_combinations(m_value + 1, vars, addToNode);
  return root;
}

AndOperator* ExactlyOperator::expand() {
  FormulaVec vars;
  std::function<Formula*(Formula*)> get_vars = [](Formula* f){ return f->get_var(); };
  transform(m_list, vars, get_vars);
  auto root = new AndOperator();
  // At most
  std::function<void(FormulaVec)> addToNode =
    [&](FormulaVec l){
    FormulaVec negl;
    negl.resize(l.size());
    std::transform(l.begin(), l.end(), negl.begin(), [](Formula* f){ return new NotOperator(f); });
    root->addChild(new OrOperator(negl));
  };
  for_all_combinations(m_value + 1, vars, addToNode);
  // At least
  addToNode =
    [&](FormulaVec l){
    root->addChild(new OrOperator(l));
  };
  for_all_combinations(vars.size() - m_value + 1, vars, addToNode);
  return root;
}

void AndOperator::tseitin(FormulaVec& clauses) {
  // X <-> AND(A1, A2, ..)
  // (X | !A1 | !A2 | ...) & (A1 | !X) & (A2 | !X) & ...
  FormulaVec first;
  first.resize(m_list.size());
  std::transform(m_list.begin(), m_list.end(), first.begin(), [](Formula* f) {
    return new NotOperator(f->get_var());
  });
  first.push_back(get_var());
  clauses.push_back(new OrOperator(first));

  auto not_this = new NotOperator(get_var());
  for (auto& f: m_list) {
    clauses.push_back(new OrOperator(FormulaVec{
      not_this, f->get_var()
    }));
  }

  // recurse down
  for (auto&f : m_list) {
    f->tseitin(clauses);
  }
}

void OrOperator::tseitin(FormulaVec& clauses) {
  // X <-> OR(A1, A2, ..)
  // (!X | A1 | A2 | ...) & (!A1 | X) & (!A2 | X) & ...
  FormulaVec first;
  first.resize(m_list.size());
  std::transform(m_list.begin(), m_list.end(), first.begin(), [](Formula* f) {
    return f->get_var();
  });
  first.push_back(new NotOperator(get_var()));
  clauses.push_back(new OrOperator(first));

  for (auto& f: m_list) {
    clauses.push_back(new OrOperator(FormulaVec{
      get_var(), new NotOperator(f->get_var())
    }));
  }

  // recurse down
  for (auto&f : m_list) {
    f->tseitin(clauses);
  }
}

void NotOperator::tseitin(FormulaVec& clauses) {
  // X <-> (!Y)
  // (!X | !Y) & (X | Y)
  auto thisVar = get_var();
  auto childVar = m_child->get_var();
  auto notThis = new NotOperator(thisVar);
  auto notChild = new NotOperator(childVar);
  clauses.push_back(new OrOperator(FormulaVec{notThis, notChild}));
  clauses.push_back(new OrOperator(FormulaVec{thisVar, childVar}));
  m_child->tseitin(clauses);
}

void ImpliesOperator::tseitin(FormulaVec& clauses) {
  // X <-> (L -> R)
  // (!X | !L | R) & (L | X) & (!R | X)
  auto thisVar = get_var();
  auto leftVar = m_left->get_var();
  auto rightVar = m_right->get_var();
  auto notThis = new NotOperator(thisVar);
  auto notLeft = new NotOperator(leftVar);
  auto notRight = new NotOperator(rightVar);
  clauses.push_back(new OrOperator(FormulaVec{notThis, notLeft, rightVar}));
  clauses.push_back(new OrOperator(FormulaVec{leftVar, thisVar}));
  clauses.push_back(new OrOperator(FormulaVec{notRight, thisVar}));

  m_left->tseitin(clauses);
  m_right->tseitin(clauses);
}

void EquivalenceOperator::tseitin(FormulaVec& clauses) {
  // X <-> (L <-> R)
  // (X | L | R) & (!X | L | R) & (X | !L | R) & (X | L | !R)
  auto thisVar = get_var();
  auto leftVar = m_left->get_var();
  auto rightVar = m_right->get_var();
  auto notThis = new NotOperator(thisVar);
  auto notLeft = new NotOperator(leftVar);
  auto notRight = new NotOperator(rightVar);
  clauses.push_back(new OrOperator(FormulaVec{thisVar, leftVar, rightVar}));
  clauses.push_back(new OrOperator(FormulaVec{notThis, leftVar, rightVar}));
  clauses.push_back(new OrOperator(FormulaVec{thisVar, notLeft, rightVar}));
  clauses.push_back(new OrOperator(FormulaVec{thisVar, leftVar, notRight}));

  m_left->tseitin(clauses);
  m_right->tseitin(clauses);
}

void AtMostOperator::tseitin(FormulaVec& clauses) {
  auto expanded = expand();
  expanded->addChild(new EquivalenceOperator(get_var(), expanded->get_var()));
  expanded->tseitin(clauses);
}

void AtLeastOperator::tseitin(FormulaVec& clauses) {
  auto expanded = expand();
  expanded->addChild(new EquivalenceOperator(get_var(), expanded->get_var()));
  expanded->tseitin(clauses);
}

void ExactlyOperator::tseitin(FormulaVec& clauses) {
  auto expanded = expand();
  expanded->addChild(new EquivalenceOperator(get_var(), expanded->get_var()));
  expanded->tseitin(clauses);
}




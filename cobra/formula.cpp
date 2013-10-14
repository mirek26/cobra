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
  return new Variable("t" + to_string(k));
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

AndOperator* NaryOperator::to_cnf_prepare(std::vector<Formula*>& out_list) {
  auto root = new AndOperator();
  for (uint i = 0; i < getChildCount(); ++i) {
    m_list[i] = m_list[i]->to_cnf();
    // build list for combinations
    if (m_list[i]->is_simple()) {
      out_list.push_back(m_list[i]);
    } else {
      // add temp var
      auto temp = get_temp();
      out_list.push_back(temp);
      root->addChild((new EquivalenceOperator(m_list[i], temp))->to_cnf());
    }
  }
  return root;
}

Formula* AtLeastOperator::to_cnf() {
  std::vector<Formula*> list;
  auto root = to_cnf_prepare(list);
  std::function<void(std::vector<Formula*>)> addToNode =
    [&](std::vector<Formula*> l){
    root->addChild(new OrOperator(l));
  };
  for_all_combinations(list.size() - m_value + 1, list, addToNode);
  return root;
}

Formula* AtMostOperator::to_cnf() {
  std::vector<Formula*> list;
  auto root = to_cnf_prepare(list);
  std::function<void(std::vector<Formula*>)> addToNode =
    [&](std::vector<Formula*> l){
    std::vector<Formula*> negl;
    negl.resize(l.size());
    std::transform(l.begin(), l.end(), negl.begin(), [](Formula* f){ return new NotOperator(f); });
    root->addChild(new OrOperator(negl));
  };
  for_all_combinations(m_value + 1, list, addToNode);
  return root;
}

Formula* ExactlyOperator::to_cnf() {
  std::vector<Formula*> list;
  auto root = to_cnf_prepare(list);
  // At most
  std::function<void(std::vector<Formula*>)> addToNode =
    [&](std::vector<Formula*> l){
    std::vector<Formula*> negl;
    negl.resize(l.size());
    std::transform(l.begin(), l.end(), negl.begin(), [](Formula* f){ return new NotOperator(f); });
    root->addChild(new OrOperator(negl));
  };
  for_all_combinations(m_value + 1, list, addToNode);
  // At least
  addToNode =
    [&](std::vector<Formula*> l){
    root->addChild(new OrOperator(l));
  };
  for_all_combinations(list.size() - m_value + 1, list, addToNode);
  return root;
}

Formula* NotOperator::to_cnf() {
  m_child = m_child->to_cnf();
  return this;
}

Formula* AndOperator::to_cnf() {
  for (auto& c: m_list) {
    c = c->to_cnf();
  }
  return this;
}

Formula* OrOperator::to_cnf() {
  for (auto& c: m_list) {
    c = c->to_cnf();
  }
  return this;
}

Formula* ImpliesOperator::to_cnf() {
  m_premise = m_premise->to_cnf();
  m_consequence = m_consequence->to_cnf();
  return this;
}

Formula* EquivalenceOperator::to_cnf() {
  m_left = m_left->to_cnf();
  m_right = m_right->to_cnf();
  return this;
}
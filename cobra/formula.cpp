/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <string>

#include "./formula.h"

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
  int childCount = m_list->getChildCount();
  for (int i = 0; i < childCount; ++i) {
    auto child = m_list->getChild(i - merged);
    auto andChild = dynamic_cast<AndOperator*>(child);
    if (andChild) {
      m_list->remove(i - merged);
      m_list->add(andChild->m_list);
      merged++;
      andChild->m_list->clear();
      delete andChild;
    }
  }
  return this;
}

Construct* OrOperator::optimize() {
  Construct::optimize();
  int merged = 0;
  int childCount = m_list->getChildCount();
  for (int i = 0; i < childCount; ++i) {
    auto child = m_list->getChild(i - merged);
    auto andChild = dynamic_cast<OrOperator*>(child);
    if (andChild) {
      m_list->remove(i - merged);
      m_list->add(andChild->m_list);
      merged++;
      andChild->m_list->clear();
      delete andChild;
    }
  }
  return this;
}


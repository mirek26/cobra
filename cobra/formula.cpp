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
  printf("%p: %s\n", this, getName().c_str());
  for (int i = 0; i < getChildCount(); ++i) {
    getChild(i)->dump(indent + 1);
  }
}

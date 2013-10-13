/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

//#include "./cobra.tab.hpp"
#include "./formula.h"

extern "C" int yyparse();

Formula* f = nullptr;

int main()
{
  yyparse();
  f->dump();
  Construct* g = f->optimize();
  g->dump();
  delete g;
  return 0;
}
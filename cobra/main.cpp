/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include "./formula.h"
#include "./util.h"

extern "C" int yyparse();

Formula* f = nullptr;

int main()
{
  yyparse();
  f->dump();
  Construct* g = f->to_cnf()->optimize();
  g->dump();
  return 0;
}
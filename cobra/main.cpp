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
  f = dynamic_cast<Formula*>(f->optimize());
  f->dump();
  Construct* g = f->to_cnf();
  g->dump();
  return 0;
}
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
  f = dynamic_cast<Formula*>(f->Simplify());
  f->dump();
  Construct* g = f->ToCnf();
  g->dump();
  return 0;
}
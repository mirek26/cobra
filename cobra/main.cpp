/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include "./formula.h"
#include "./util.h"

extern "C" int yyparse();

AstManager m;

int main()
{
  yyparse();
  auto f = dynamic_cast<Formula*>(m.last()->Simplify());
  f->dump();
  Construct* g = f->ToCnf();
  g->dump();
  m.deleteAll();
  return 0;
}
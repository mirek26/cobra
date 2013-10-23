/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include "formula.h"
#include "util.h"
#include "solver.h"

extern "C" int yyparse();

AstManager m;

int main()
{
  yyparse();
  auto f = dynamic_cast<Formula*>(m.last()->Simplify());
  f->dump();

  Solver s;
  s.AddConstraint(f);
  bool sat = s.Satisfiable();
  printf(sat ? "Jo.\n" : "Ne.\n" );

  m.deleteAll();
  return 0;
}
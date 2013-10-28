/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include "formula.h"
#include "util.h"
#include "solver.h"
#include "ast-manager.h"

extern "C" int yyparse();

AstManager m;

int main()
{
  yyparse();
  auto f = dynamic_cast<Formula*>(m.last()->Simplify());

  Solver s;
  s.AddConstraint(f);

  printf("INPUT: %s\n", f->pretty().c_str());
  f->dump();
  printf("\nCNF: %s\n", s.formula()->pretty().c_str());

  bool sat = s.Satisfiable();
  if (sat) s.PrintAssignment();
  printf(sat ? "Jo.\n" : "Ne.\n" );

  m.deleteAll();
  return 0;
}
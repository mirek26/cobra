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
  m.init()->dump();
  auto f = dynamic_cast<Formula*>(m.init()->Simplify());

  Solver s;
  s.AddConstraint(f);

  printf("INPUT: %s\n", f->pretty().c_str());
  printf("CNF: %s\n", s.formula()->pretty().c_str());

  bool sat = s.Satisfiable();
  printf(sat ? "\nSATISFIABLE.\n" : "\nNON-SATISFIABLE.\n" );
  if (sat) s.PrintAssignment();

  m.deleteAll();
  return 0;
}
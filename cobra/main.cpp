/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include "formula.h"
#include "util.h"
#include "solver.h"
#include "parser.h"

extern "C" int yyparse();

Parser m;

int main()
{
  yyparse();
  auto f = dynamic_cast<Formula*>(m.init()->Simplify());

  printf("LOADED.\n");

  Solver s;
  s.AddConstraint(f);

  printf("INPUT: %s\n", f->pretty().c_str());
  printf("CNF: %s\n", s.formula()->pretty().c_str());

  bool sat = s.Satisfiable();
  printf(sat ? "\nSATISFIABLE.\n" : "\nNON-SATISFIABLE.\n" );
  if (sat) s.PrintAssignment();

  //m.deleteAll();
  return 0;
}
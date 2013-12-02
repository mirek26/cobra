/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <set>

#include "formula.h"
#include "util.h"
#include "parser.h"

extern "C" int yyparse();
extern Parser m;

int main()
{
  yyparse();
  Game& g = m.game();
  //g.setInit(Formula::Parse("a & b | c"));
  //if (!g.complete()) exit(1);
  //
  g.doAll();
  //m.deleteAll();
  return 0;
}
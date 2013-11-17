/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "ast-manager.h"
#include "formula.h"
#include "game.h"

void Construct::dump(int indent) {
  for (int i = 0; i < indent; ++i) {
    printf("   ");
  }
  printf("%p: %s\n", (void*)this, name().c_str());
  for (uint i = 0; i < child_count(); ++i) {
    child(i)->dump(indent + 1);
  }
}

// Overloaded insance of get method for a Variable.
// It first looks into the variable map and creates a new one
// only if it isn't there.
Variable* AstManager::get(identity<Variable>, const std::string& ident) {
  if (variables_.count(ident) > 0) {
    return variables_[ident];
  } else {
    auto node = new Variable(ident);
    nodes_.push_back(node);
    last_ = node;
    variables_[ident] = node;
    return node;
  }
}

std::vector<Variable*>* AstManager::getVariableRange(Variable* from,
                                                     Variable* to) {
  auto vars = new std::vector<Variable*>();
  if (from->ident() != to->ident()) {
    throw new ParserException("Invalid range.");
  }

  for (int i = from->index(); i <= to->index(); i++) {
    vars->push_back(get<Variable>(from->ident(), i));
  }
  return vars;
}

void AstManager::addExp(Experiment* exp) {
  exp->dump();
}


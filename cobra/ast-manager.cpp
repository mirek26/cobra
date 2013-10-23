/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "ast-manager.h"
#include "formula.h"

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
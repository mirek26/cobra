/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include <map>
#include "formula.h"
#include "game.h"

#include "parser.h"

Parser m;

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
Variable* Parser::get(identity<Variable>, const std::string& ident) {
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

Variable* Parser::get(identity<Variable>,
                      const std::string& ident,
                      const std::vector<int>& incides) {
  std::string fullIdent = ident + Variable::joinIndices(incides);
  if (variables_.count(fullIdent) > 0) {
    return variables_[fullIdent];
  } else {
    auto node = new Variable(ident, incides);
    nodes_.push_back(node);
    last_ = node;
    variables_[fullIdent] = node;
    return node;
  }
}

void Parametrization::ForAll(std::function<void(std::vector<Variable*>&)> call,
                             std::map<int, int> equiv,
                             uint n) {
  static std::set<Variable*> used;
  static std::vector<Variable*> comb;
  if (n == size()) {
    call(comb);
  } else {
    std::set<int> equiv_classes;
    for (auto& v: *this->at(n)) {
      int equiv_class = equiv.count(v->id()) ? equiv[v->id()] : -1;
      if (used.count(v) || equiv_classes.count(equiv_class)) {
        continue;
      }
      comb.push_back(v);
      used.insert(v);
      equiv_classes.insert(equiv_class);
      ForAll(call, equiv, n+1);
      used.erase(v);
      comb.pop_back();
    }
  }
}

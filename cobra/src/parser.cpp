/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "./parser.h"

#include <vector>
#include <map>
#include "./formula.h"
#include "./game.h"

Parser m;

void Parser::deleteAll() {
  for (auto& n : nodes_) delete n;
  nodes_.clear();
}

// Overloaded insance of get method for a Variable.
// It first looks into the variable map and creates a new one
// only if it isn't there.
// Variable* Parser::get(identity<Variable>, const string& ident) {
//   if (variables_.count(ident) > 0) {
//     return variables_[ident];
//   } else {
//     auto node = new Variable(ident);
//     nodes_.push_back(node);
//     last_ = node;
//     variables_[ident] = node;
//     return node;
//   }
// }

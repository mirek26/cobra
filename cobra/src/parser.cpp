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

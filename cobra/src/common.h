/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <iostream>

#ifndef COBRA_SRC_COMMON_H_
#define COBRA_SRC_COMMON_H_

/*
var(s) - variable(s)
param(s) - parametrization(s)
gen - generate
equiv - equivalence
*/

// common types
typedef unsigned int uint;  // TODO(myreg): zkusit uint fast
typedef unsigned char CharId;
typedef int VarId;  // VarId must se signed, -1 denotes negation of var 1
typedef unsigned char MapId;

// type aliases for basic STL types
typedef std::string string;
template<typename T> using vec = std::vector<T>;
template<typename T> using set = std::set<T>;

template<typename T>
struct identity { typedef T type; };

namespace vertex_type {
  const int
    kKnowledgeRoot = -1,
    kOutcomeRoot = -2,
    kVariableId = 3,
    kTrueVar = 4,
    kFalseVar = 5,
    kNotId = 6,
    kImpliesId = 10,
    kEquivalenceId = 11,
    kAndId = 12,
    kOrId = 13,
    kMappingId = 14,
    kAtLeastId = 20,
    kAtMostId = 21,
    kExactlyId = 22;
}  // namespace vertex_type

namespace color {
  extern const string head;
  extern const string emph;
  extern const string normal;
  extern const string error;
  extern const char* const shead;
  extern const char* const semph;
  extern const char* const snormal;
  extern const char* const serror;
}

template<class T>
void for_all_combinations(int k,
                          const vec<T>& list,
                          std::function<void(vec<T>)> action,
                          int offset = 0) {
  assert(k >= 0);
  static vec<T> combination;
  if (k == 0) {
    action(combination);
    return;
  }
  for (uint i = offset; i <= list.size() - k; ++i) {
    combination.push_back(list[i]);
    for_all_combinations(k-1, list, action, i+1);
    combination.pop_back();
  }
}

template<class T, class R>
void transform(const vec<T>& from, const vec<R>& to, std::function<R(T)>& fun) {
  to.resize(from.size());
  std::transform(from.begin(), from.end(), to.begin(), fun);
}

vec<string> split(string s);
bool readIntOrString(uint& i, string& str);
double toSeconds(clock_t time);

#endif  // COBRA_SRC_COMMON_H_

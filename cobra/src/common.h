/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cassert>
#include <string>
#include <vector>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <functional>
#include <iostream>

#ifndef COBRA_COMMON_H_
#define COBRA_COMMON_H_

/*
var(s) - variable(s)
param(s) - parametrization(s)
gen - generate
equiv - equivalence
*/

// common types
typedef unsigned int uint;
typedef unsigned char CharId;
typedef int VarId; // must se signed, -1 denotes negation of var 1
typedef unsigned char MapId;

// type aliases for
typedef std::string string;
template<typename T> using vec = std::vector<T>;
//template<typename T> using set = std::unordered_set<T>;
template<typename T> using set = std::set<T>;

template<typename T>
struct identity { typedef T type; };

namespace vertex_type {
  const int
    kOutcomeRoot = -2,
    kKnowledgeRoot = -1,
    kVariableId = 1,
    kNotId = 2,
    kImpliesId = 3,
    kEquivalenceId = 4,
    kAndId = 5,
    kOrId = 6,
    kMappingId = 7,
    kAtLeastId = 8,
    kAtMostId = 9,
    kExactlyId = 10;
}

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
void for_all_combinations(int k, vec<T>& list, std::function<void(vec<T>)> action, int offset = 0);

template<class T>
void for_all_combinations(int k, vec<T>& list, std::function<void(vec<T>)> action, int offset) {
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
void transform(vec<T>& from, vec<R>& to, std::function<R(T)>& fun) {
  to.resize(from.size());
  std::transform(from.begin(), from.end(), to.begin(), fun);
}

vec<string> split(string s);
bool readIntOrString(uint& i, string& str);
double toSeconds(clock_t time);

#endif  // COBRA_COMMON_H_

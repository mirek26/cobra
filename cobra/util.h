/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#ifndef COBRA_UTIL_H_
#define COBRA_UTIL_H_

typedef unsigned int uint;

// convert int to string, equivalent to std::to_string (for llvm/clang)
std::string to_string(int n);

template<class T>
void for_all_combinations(int k, std::vector<T>& list, std::function<void(std::vector<T>)> action, int offset = 0);

template<class T>
void for_all_combinations(int k, std::vector<T>& list, std::function<void(std::vector<T>)> action, int offset) {
  assert(k >= 0);
  static std::vector<T> combination;
  if (k == 0) {
    action(combination);
    return;
  }
  for (int i = offset; i <= list.size() - k; ++i) {
    combination.push_back(list[i]);
    for_all_combinations(k-1, list, action, i+1);
    combination.pop_back();
  }
}

template<class T, class R>
void transform(std::vector<T>& from, std::vector<R>& to, std::function<R(T)>& fun) {
  to.resize(from.size());
  std::transform(from.begin(), from.end(), to.begin(), fun);
}

#endif  // COBRA_UTIL_H_

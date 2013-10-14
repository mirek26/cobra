/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "util.h"

std::string to_string(int n) {
  std::ostringstream convert;
  convert << n;
  return convert.str();
}

template<class T>
void for_all_combinations(int k, std::vector<T>& list, std::function<void(std::vector<T>)> action, int offset) {
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
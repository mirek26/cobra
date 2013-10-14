/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
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

#endif  // COBRA_UTIL_H_

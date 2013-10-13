/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <string>

#ifndef COBRA_UTIL_H_
#define COBRA_UTIL_H_

typedef unsigned int uint;

// convert int to string, equivalent to std::to_string (for llvm/clang)
std::string to_string(int n);

#endif  // COBRA_UTIL_H_

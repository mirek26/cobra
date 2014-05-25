/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include "./common.h"

namespace color {
  const string head = "\033[1;97m";
  const string emph = "\033[32m";
  const string normal = "\033[0m";
  const string error = "\033[31m";
  const char* const shead = "\033[1;97m";
  const char* const semph = "\033[32m";
  const char* const snormal = "\033[0m";
  const char* const serror = "\033[31m";
}

void print_head(string name) {
  printf("\n%s===== %s =====%s\n", color::shead, name.c_str(), color::snormal);
}

string toUpper(string s) {
  std::transform(s.begin(), s.end(), s.begin(), toupper);
  return s;
}

bool readIntOrString(uint& i, string& str) {
  std::cin >> str;
  if (std::cin.fail() || std::cin.eof()) {
    printf("\n(end of input)\n");
    throw std::runtime_error("Unexpected end of file.");
  }
  try {
    i = std::stoi(str);
    return true;
  } catch (std::invalid_argument) { }
  return false;
}

double toSeconds(clock_t time) {
  return static_cast<double>(time)/CLOCKS_PER_SEC;
}

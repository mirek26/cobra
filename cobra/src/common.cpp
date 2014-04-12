/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "common.h"

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

vec<string> split(string s) {
  vec<string> result;
  string delimiter = " ";
  size_t pos = 0;
  string token;
  while ((pos = s.find(delimiter)) != string::npos) {
      result.push_back(s.substr(0, pos));
      s.erase(0, pos + delimiter.length());
  }
  return result;
}

bool readIntOrString(uint& i, string& str) {
  std::cin >> str;
  if (std::cin.fail() || std::cin.eof()) {
    printf("\n(end of input)\n");
    exit(0);
  }
  try {
    i = std::stoi(str);
    return true;
  } catch (std::invalid_argument) { }
  return false;
}
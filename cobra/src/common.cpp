/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "common.h"

std::vector<std::string> split(std::string s) {
  std::vector<std::string> result;
  std::string delimiter = " ";
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos) {
      result.push_back(s.substr(0, pos));
      s.erase(0, pos + delimiter.length());
  }
  return result;
}

bool readIntOrString(int& i, std::string& str) {
  std::cin >> str;
  try {
    i = std::stoi(str);
    return true;
  } catch (std::invalid_argument) { }
  if (std::cin.eof()) {
    printf("\n(end of input)\n");
    exit(0);
  }
  return false;
}
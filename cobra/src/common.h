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

// common types
typedef unsigned int uint;
typedef unsigned char CharId;
typedef int VarId;  // VarId must se signed, -1 denotes negation of var 1
typedef unsigned char MapId;

// type aliases for basic STL types
typedef std::string string;
template<typename T> using vec = std::vector<T>;
template<typename T> using set = std::set<T>;

/**
 * Helper structure for command line arguments
 */
typedef struct Args {
  string filename;
  string mode;
  string backend;
  string stg_experiment;
  string stg_outcome;
  bool symmetry_detection;
  double opt_bound;
} Args;

template<typename T>
struct identity { typedef T type; };

/**
 * Vertex labels for experiment graph construction.
 */
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

/**
 * ASCII escape color codes for colored output.
 */
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

/**
 * Prints header of a section.
 */
void print_head(string name);

/**
 * Convert a string to upper case.
 */
string toUpper(string str);

/**
 * Reads a number OR string from the standard input, return true if successful
 */
bool readIntOrString(uint& i, string& str);

/**
 * Converts time from clock_t to double.
 */
double toSeconds(clock_t time);

#endif  // COBRA_SRC_COMMON_H_

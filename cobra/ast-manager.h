/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */

#include <vector>
#include "util.h"

#ifndef COBRA_AST_MANAGER_H_
#define COBRA_AST_MANAGER_H_

// Base class for AST node
class Construct {
  const int kChildCount;

 public:
  explicit Construct(int childCount)
      : kChildCount(childCount) { }

  virtual ~Construct() { }
  virtual uint child_count() { return kChildCount; }
  virtual Construct* child(uint nth) = 0;
  virtual void set_child(uint nth, Construct* value) { assert(child(nth) == value); }
  virtual std::string name() = 0;
  virtual void dump(int indent = 0);

  virtual Construct* Simplify() {
    for (uint i = 0; i < child_count(); ++i) {
      set_child(i, child(i)->Simplify());
    }
    return this;
  }
};

class AstManager {
  std::vector<Construct*> nodes_;
  Construct* last_;

 public:
  template<class T, typename... Ts>
  T* get(const Ts&... ts) {
    T* node = new T(ts...);
    nodes_.push_back(node);
    last_ = node;
    return node;
  }

  void deleteAll() {
    for (auto& n: nodes_) delete n;
    nodes_.clear();
  }

  Construct* last() {
    return last_;
  }
};

#endif   // COBRA_AST_MANAGER_H_
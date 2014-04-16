
#include "formula.h"
#include "simple-solver.h"


void SimpleSolver::PrintAssignment() {
  assert(!sat_.empty());
  auto& a = codes_[sat_[0]];
  vec<int> trueVar;
  vec<int> falseVar;
  for (uint id = 1; id <= vars_.size(); id++) {
    if (a[id]) trueVar.push_back(id);
    else falseVar.push_back(id);
  }
  printf("TRUE: ");
  for (auto s: trueVar) printf("%s ", vars_[s-1]->ident().c_str());
  printf("\nFALSE: ");
  for (auto s: falseVar) printf("%s ", vars_[s-1]->ident().c_str());
  printf("\n");
}

void SimpleSolver::Update() {
  sat_.clear();
  for (uint i = 0; i < codes_.size(); i++) {
    bool ok = true;
    for (auto& constr: constraints_) {
      if (!constr.first->Satisfied(codes_[i], constr.second)) {
        ok = false;
        break;
      }
    }
    if (ok) sat_.push_back(i);
  }
}

string SimpleSolver::pretty() {
  string s = restriction_->pretty(false) + " & ";
  for (auto& c: constraints_) {
    s += c.first->pretty(false, &c.second) + " & ";
  }
  s.erase(s.length()-3, 3);
  return s;
}
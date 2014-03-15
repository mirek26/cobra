/*
 * Copyright 2014, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "formula.h"
#include "common.h"

extern "C" {
  #include <picosat/picosat.h>
}

#include "cnf-formula.h"

//------------------------------------------------------------------------------
// Adding constraints

// Variable* CnfFormula::getVariable(int id) {
//   return (variables_.count(id) > 0 ? variables_[id] : nullptr);
// }

// // adds variable to variables_ and original_
// void CnfFormula::addVariable(Variable* var) {
//   assert(var);
//   variables_[var->id()] = var;
//   if (var->orig()) {
//     original_.insert(var->id());
//   }
// }

// void CnfFormula::addLiteral(TClause& clause, Formula* literal) {
//   assert(literal->isLiteral());
//   auto var = dynamic_cast<Variable*>(literal);
//   auto neg = false;
//   if (!var) {
//     var = dynamic_cast<Variable*>(literal->neg());
//     neg = true;
//   }
//   addVariable(var);
//   // add to the last clause
//   int add = neg ? -var->id(): var->id();
//   clause.insert(add);
//   picosat_add(picosat_, add);
// }

// void CnfFormula::addClause(std::vector<Formula*>* list) {
//   TClause c;
//   for (auto f: *list) {
//     addLiteral(c, f);
//   }
//   clauses_.insert(c);
//   picosat_add(picosat_, 0);
// }

// void CnfFormula::addClause(std::initializer_list<Formula*> list) {
//   TClause c;
//   for (auto f: list) {
//     addLiteral(c, f);
//   }
//   clauses_.insert(c);
//   picosat_add(picosat_, 0);
// }


void CnfFormula::addClause(std::vector<VarId>& list) {
  Clause c(list.begin(), list.end());
  clauses_.insert(c);
}

void CnfFormula::addClause(std::initializer_list<VarId> list) {
  Clause c(list.begin(), list.end());
  clauses_.insert(c);
}

void CnfFormula::AddConstraint(Formula* formula) {
  assert(formula);
  CnfFormula* cnf = formula->ToCnf();
  assert(cnf);
  AddConstraint(*cnf);
  delete cnf;
}

void CnfFormula::AddConstraint(Formula* formula, std::vector<CharId> params) {
  assert(formula);
  CnfFormula* cnf = formula->ToCnf(params);
  assert(cnf);
  AddConstraint(*cnf);
  delete cnf;
}

void CnfFormula::AddConstraint(CnfFormula& cnf) {
  clauses_.insert(cnf.clauses_.begin(), cnf.clauses_.end());
}

//------------------------------------------------------------------------------
// Pretty print

// std::string CnfFormula::pretty_literal(int id, bool unicode) {
//   assert(variables_.count(abs(id)) == 1);
//   return (id > 0 ? variables_[id] : variables_[-id]->neg())->pretty(unicode);
// }

std::string CnfFormula::pretty_clause(const Clause& clause) {
  if (clause.empty()) return "()";
  std::string s = "(" + std::to_string(*clause.begin());
  for (auto it = std::next(clause.begin()); it != clause.end(); ++it) {
    s += " | " + std::to_string(*it);
  }
  s += ")";
  return s;
}

std::string CnfFormula::pretty() {
  if (clauses_.empty()) return "(empty)";
  std::string s = pretty_clause(*clauses_.begin());
  for (auto it = std::next(clauses_.begin()); it != clauses_.end(); ++it) {
    s += "&" + pretty_clause(*it);
  }
  return s;
}

//------------------------------------------------------------------------------
// SAT solver stuff

uint CnfFormula::GetFixedVariables() {
  uint r = 0;
  // for (auto& var: original_) {
  //   picosat_assume(picosat_, var);
  //   if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) r++;
  //   picosat_assume(picosat_, -var);
  //   if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) r++;
  // }
  return r;
}

uint CnfFormula::GetFixedPairs() {
  uint r = 0;
  // for (auto& var1: original_) {
  //   for (auto& var2: original_) {
  //     picosat_assume(picosat_, var1);
  //     picosat_assume(picosat_, var2);
  //     if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) r++;
  //     picosat_assume(picosat_, -var1);
  //     picosat_assume(picosat_, var2);
  //     if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) r++;
  //     picosat_assume(picosat_, var1);
  //     picosat_assume(picosat_, -var2);
  //     if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) r++;
  //     picosat_assume(picosat_, -var1);
  //     picosat_assume(picosat_, -var2);
  //     if (picosat_sat(picosat_, -1) == PICOSAT_UNSATISFIABLE) r++;
  //   }
  // }
  return r;
}

void CnfFormula::InitSolver() {
  picosat_ = picosat_init();
  for (auto& c: clauses_) {
    for (auto n: c) {
      picosat_add(picosat_, n);
    }
    picosat_add(picosat_, 0);
  }
}

bool CnfFormula::Satisfiable() {
  auto result = picosat_sat(picosat_, -1);
  return (result == PICOSAT_SATISFIABLE);
}

void CnfFormula::PrintAssignment(std::vector<Variable*>& vars) {
  std::vector<int> trueVar;
  std::vector<int> falseVar;
  for (uint id = 1; id <= vars.size(); id++) {
    if (picosat_deref(picosat_, id) == 1) trueVar.push_back(id);
    else falseVar.push_back(id);
  }
  printf("TRUE: ");
  for (auto s: trueVar) printf("%s ", vars[s-1]->ident().c_str());
  printf("\nFALSE: ");
  for (auto s: falseVar) printf("%s ", vars[s-1]->ident().c_str());
  printf("\n");
}

//------------------------------------------------------------------------------
// Symmetry-breaking stuff

bool CnfFormula::ProbeEquivalence(const Clause& clause, VarId var1, VarId var2) {
  Clause test(clause);
  bool p1 = test.erase(var1);
  bool p2 = test.erase(var2);
  bool n1 = test.erase(-var1);
  bool n2 = test.erase(-var2);
  if (p1) test.insert(var2);
  if (p2) test.insert(var1);
  if (n1) test.insert(-var2);
  if (n2) test.insert(-var1);
  return clauses_.count(test);
}

// Compute 'syntactical' equivalence on variables with ids 1 - limit; how about semantical?
std::vector<int> CnfFormula::ComputeVariableEquivalence(VarId limit) {
  std::vector<int> components(limit + 1, 0);
  int cid = 1;
  for (VarId i = 1; i <= limit; i++) {
    if (components[i] > 0) continue;
    components[i] = cid++;
    for (VarId j = i + 1; j <= limit; j++) {
      if (components[j] > 0) continue;
      bool equiv = true;
      for (auto& clause: clauses_) {
        equiv = equiv && ProbeEquivalence(clause, i, j);
        if (!equiv) break;
      }
      if (equiv) components[j] = components[i];
    }
  }
  return components;
}

// CnfFormula CnfFormula::SubstituteParams(std::vector<Variable*> params) {
//   CnfFormula result;
//   bool skipClause;
//   bool skipLiteral;
//   for (auto var: params) {
//     result.addVariable(var);
//   }
//   // copy all clauses:
//   for (auto& clause: clauses_) {
//     TClause c;
//     skipClause = false;
//     for (auto lit: clause) {
//       uint id = abs(lit);
//       skipLiteral = false;
//       for (auto p: paramEq_) {
//         if (p->tseitin_id() == id && p->param() < params.size()) {
//           if ((params[p->param()] == p->var()) == (lit > 0)) {
//             skipClause = true; // clause satisfied by lit
//           } else {
//             skipLiteral = true; // literal is false
//           }
//           break;
//         }
//       }
//       if (skipLiteral) continue;
//       assert(variables_.count(id) == 1);
//       auto var = variables_[id];
//       result.addVariable(var);
//       auto param = var->getParam();
//       if (param != 0 && param <= params.size()) {
//         lit = (lit > 0 ? 1 : -1) * params[param-1]->id();
//       }
//       c.insert(lit);
//     }

//     if (!skipClause) {
//       result.clauses_.insert(c);
//     }
//   }
//   result.ResetPicosat();
//   return result;
// }

// void CnfFormula::ResetPicosat() {
//   if (picosat_) picosat_reset(picosat_);
//   picosat_ = picosat_init();
//   for (auto& clause: clauses_) {
//     for (auto lit: clause) {
//       picosat_add(picosat_, lit);
//     }
//     picosat_add(picosat_, 0);
//   }
// }


// void CnfFormula::BuildBlissGraph(bliss::Digraph& g) {
//   auto vars = g.get_nof_vertices() / 2;

//   std::map<uint, uint> var_ids;
//   // Go through all outcome formulas and create nodes.
//   std::function<void(Formula*)> CreateNodes = [&](Formula* c) {
//     if (c->isLiteral()) return;
//     if (node_ids.count(c) == 0) {
//       node_ids[c] = new_id++;
//       // printf("add VERTEX %i for %s\n", new_id-1, c->pretty().c_str());
//       g->add_vertex(max_group_id + c->node_id());
//     }
//     for (uint i = 0; i < c->child_count(); i++) CreateNodes(c->child(i));
//   };
//   for (auto outcome: outcomes_) {
//     CreateNodes(outcome);
//   }
//   // Create all other edges according to the structure of the formula
//   std::function<void(Formula*)> AddEdges = [&](Formula* c) {
//     for (uint i = 0; i < c->child_count(); i++) {
//       auto ch = c->child(i);
//       bool neg = false;
//       if (ch->isLiteral()) {
//         if (dynamic_cast<NotOperator*>(ch)) {
//           neg = true;
//           ch = ch->neg();
//         }
//         auto map = dynamic_cast<Mapping*>(ch);
//         auto var = dynamic_cast<Variable*>(ch);
//         assert(map || var);
//         // printf("ADD edge %i %i for %s\n", node_ids[c], neg + (map ? var_ids[map->getValue(params)]
//         //                                      : var_ids[var_ids[var->id()]]), c->pretty().c_str());
//         g->add_edge(node_ids[c], neg + (map ? var_ids[map->getValue(params)]
//                                             : var_ids[var_ids[var->id()]]));
//       } else {
//         // printf("add edge %i %i for %s to %s.\n", node_ids[c], node_ids[ch], c->pretty().c_str(), ch->pretty().c_str());
//         g->add_edge(node_ids[c], node_ids[ch]);
//         AddEdges(ch);
//       }
//     }
//   };
//   for (auto outcome: outcomes_) {
//     AddEdges(outcome);
//   }
//   g->set_splitting_heuristic(bliss::Digraph::shs_fsm);
//   return g;
// }



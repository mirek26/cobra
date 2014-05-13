/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include "common.h"

#ifndef COBRA_SOLVER_H_
#define COBRA_SOLVER_H_

class Variable;
class Formula;
class Solver;

/**
 * Storage for statistics about a SAT solver. There are 3 categories
 * of operations:
 *  - fixed (resolving fixed variables)
 *  - sat (resolving satisfiability)
 *  - models (model counting)
 * For each category, we store number of calls and the total time.
 */
typedef struct SolverStats {
  clock_t fixed_time = 0;
  int fixed_calls = 0;
  clock_t sat_time = 0;
  int sat_calls = 0;
  clock_t models_time = 0;
  int models_calls = 0;
} SolverStats;

/**
 * Abstract class for a SAT solver.
 * Most public functions are only time-measuring wrappers that only call
 * their protected variant with "_" prefix. These protected functions
 * are virtual and should be (re)implemented in a derived class.
 */
class Solver {
 protected:
  uint var_count_;

 public:
  virtual ~Solver() { }

  /**
   * Gets statistics about solver usage.
   */
  virtual SolverStats& stats() = 0;

  /**
   * Adds a non-parametrized formula as a constraint.
   */
  virtual void AddConstraint(Formula* formula) = 0;

  /**
   * Adds a parametrized formula as a constraint.
   */
  virtual void AddConstraint(Formula* formula, const vec<CharId>& params) = 0;

  /**
   * Opens a new context. All constraints added in this context
   * are attached to it and discared when the context is closed
   * with 'CloseContext'. Contexts can be arbitrarily nested.
   */
  virtual void OpenContext() = 0;

  /**
   * Closes a context opened by 'OpenContext'. All constraints added
   * since the 'OpenContext' call are discarded.
   */
  virtual void CloseContext() = 0;

  /**
   * Returns true if and only if there is no valuation of the variables
   * that satisfies current constraints and variable 'id' is false.
   * Time-measuring wrapper.
   */
  bool MustBeTrue(VarId id);

  /**
   * Returns true if and only if there is no valuation of the variables
   * that satisfies current constraints and variable 'id' is true.
   * Time-measuring wrapper.
   */
  bool MustBeFalse(VarId id);

  /**
   * Returns a vector of variables that are fixed by current constraints,
   * i.e. in all models of current constainsts, the variable has the same value.
   * The resulting vector contains a variable's id, if it must be true, and
   * its negation, if it must be false.
   * Time-measuring wrapper.
   */
  vec<VarId> GetFixedVars();

  /**
   * Returns the number of variables that are fixed by current constraints
   * (see GetFixedVars).
   * Time-measuring wrapper.
   */
  uint GetNumOfFixedVars();

  /**
   * Returns true if and only if all current constraints can be satisfied.
   * Time-measuring wrapper.
   */
  bool Satisfiable();

  /**
   * This can be called only right after a successful 'Satisfiable' call.
   * Returns true if the current constraints have only one model.
   * Time-measuring wrapper.
   */
  bool OnlyOneModel();

  /**
   * Returns the number of models of the current constraints.
   * Time-measuring wrapper.
   */
  uint NumOfModels();

  /**
   * Generates all models of the current constraints.
   * Time-measuring wrapper.
   */
  vec<vec<bool>> GenerateModels();

  /**
   * Retrieves the model after a successful 'Satisfiable' call.
   */
  virtual vec<bool> GetModel() = 0;

 protected:

  virtual bool _MustBeTrue(VarId id) = 0;
  virtual bool _MustBeFalse(VarId id) = 0;
  virtual vec<VarId> _GetFixedVars() = 0;
  virtual uint _GetNumOfFixedVars() = 0;
  virtual bool _Satisfiable() = 0;
  virtual bool _OnlyOneModel() = 0;
  virtual uint _NumOfModels() = 0;
  virtual vec<vec<bool>> _GenerateModels() = 0;
};

/**
 * Abstract class for a SAT solver taking constraints in CNF form.
 */
class CnfSolver: public Solver {
  const vec<CharId>* build_for_params_ = nullptr;

 public:
  /**
   * Gets a parametrization for an ongoing Tseitin transformation,
   * invalid otherwise.
   */
  const vec<CharId>* build_for_params() const { return build_for_params_; }

  /**
   * Adds a clause (disjunction of given variables) as a constraint.
   */
  virtual void AddClause(vec<VarId>& list) = 0;
  virtual void AddClause(std::initializer_list<VarId> list) = 0;

  /**
   * General constraints are added by Tseitin tranformation to CNF.
   */
  void AddConstraint(Formula* formula);
  void AddConstraint(Formula* formula, const vec<CharId>& params);

  /**
   * Generic implementation of _OnlyOneModel by adding a clause
   * that excludes the current model.
   */
  virtual bool _OnlyOneModel();

  /**
   * Gets an id of a fresh variable, needed during Tseitin tranformation.
   */
  virtual VarId NewVarId() = 0;
};

#endif   // COBRA_SOLVER_H_

/*
 * Copyright (c) 2014, Miroslav Klimos <miroslav.klimos@gmail.com>
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "solver.h"
#include "formula.h"

void CnfSolver::AddConstraint(Formula* formula) {
  assert(formula);
  formula->ResetTseitinIds();
  formula->TseitinTransformation(*this, true);
}

void CnfSolver::AddConstraint(Formula* formula, const vec<CharId>& params) {
  build_for_params_ = &params;
  AddConstraint(formula);
  build_for_params_ = nullptr;
}


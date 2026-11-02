/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Elimination of the equalities of a CubeProblem with Griggio's method, so
 * that the unit cube test can be applied to the result.
 */

#include "cvc5_private.h"

#pragma once

#include <utility>
#include <vector>

#include "theory/arith/linear/cube/unit_cube.h"
#include "util/integer.h"

namespace cvc5::internal {
namespace theory {
namespace arith::linear {

/** The value d_constant + sum_j a_j y_j, for each (j, a_j) in d_coeffs. */
struct AffineExpr
{
  Integer d_constant;
  std::vector<std::pair<size_t, Integer>> d_coeffs;

  /** The value for the given values of the variables y. */
  Integer evaluate(const std::vector<Integer>& y) const;
};

/** The outcome of eliminateEqualities. */
struct EliminationResult
{
  enum class Status
  {
    /** d_reduced and d_map are set. */
    REDUCED,
    /**
     * The equalities have no integer solution, or none that satisfies the
     * other bounds.
     */
    INFEASIBLE,
    /** The step limit was reached. */
    UNKNOWN
  };
  Status d_status = Status::UNKNOWN;
  /**
   * A problem without equalities over new variables y. It has an integer
   * solution y iff p has the integer solution x = d_map(y).
   */
  CubeProblem d_reduced;
  /** Each input variable of p as an expression over the inputs of d_reduced. */
  std::vector<AffineExpr> d_map;
  /** The number of equalities of p. */
  size_t d_equalities = 0;
  /** The number of fresh variables introduced. */
  size_t d_fresh = 0;
};

/**
 * Eliminates the equalities of p, i.e., its variables with equal lower and
 * upper bounds, following the Diophantine equation handler of Griggio (JSAT
 * 2012) as implemented in cvc5's DioSolver: the equalities are solved one at
 * a time for a variable with a coefficient of minimal absolute value; if it
 * is not 1, the equality is first decomposed with a fresh variable. The
 * solution x = d_map(y) is substituted into the remaining bounds of p.
 *
 * Unlike DioSolver, this works on a copy of the problem and does not produce
 * conflicts, lemmas or proofs. stepLimit bounds the number of solved and
 * decomposed equalities.
 */
EliminationResult eliminateEqualities(const CubeProblem& p,
                                      uint64_t stepLimit);

/** The expressions of map with y replaced by inner(y). */
std::vector<AffineExpr> composeMaps(const std::vector<AffineExpr>& map,
                                    const std::vector<AffineExpr>& inner);

/** The value of every expression of map for the given values of y. */
std::vector<Integer> applyMap(const std::vector<AffineExpr>& map,
                              const std::vector<Integer>& y);

}  // namespace arith::linear
}  // namespace theory
}  // namespace cvc5::internal

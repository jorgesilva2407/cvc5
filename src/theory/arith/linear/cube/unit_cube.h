/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The unit cube test (Bromberger and Weidenbach, IJCAR 2016) and the
 * detection of implied equalities, on a snapshot of the linear problem.
 */

#include "cvc5_private.h"

#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "theory/arith/delta_rational.h"
#include "util/integer.h"
#include "util/rational.h"

namespace cvc5::internal {
namespace theory {
namespace arith::linear {

/**
 * A copy of the current linear problem, independent of the solver state.
 *
 * Variables are numbered 0..n-1 for the inputs and n..n+m-1 for the rows,
 * where row i defines variable n+i as a linear combination of inputs.
 */
struct CubeProblem
{
  /** The number of input variables. */
  size_t d_numInputs = 0;
  /** The rows, as coefficients over input variables. */
  std::vector<std::vector<std::pair<size_t, Rational>>> d_rows;
  /** Bounds, indexed by variable (inputs, then rows). */
  std::vector<std::optional<DeltaRational>> d_lower;
  std::vector<std::optional<DeltaRational>> d_upper;

  size_t numVariables() const { return d_numInputs + d_rows.size(); }
  /** Adds an input variable and returns it. */
  size_t addInput();
  /** Adds a row over input variables and returns its variable. */
  size_t addRow(std::vector<std::pair<size_t, Rational>> coeffs);
  /** The shift 1/2 ||a||_1 of variable v (1/2 for inputs). */
  Rational shift(size_t v) const;
  /** The value of every variable given values of the inputs. */
  std::vector<Rational> evaluate(const std::vector<Integer>& inputs) const;
  /** Whether some variable has equal lower and upper bounds. */
  bool hasEquality() const;
};

/** The outcome of the unit cube test. */
struct CubeResult
{
  enum class Status
  {
    /** d_point is an integer point satisfying all bounds. */
    SUCCESS,
    /** Shifting makes the bounds of some variable cross. */
    NO_ROOM,
    /** The shifted system is infeasible. */
    SHIFTED_INFEASIBLE,
    /** The pivot limit was reached. */
    UNKNOWN,
    /** The rounded point violates a bound (should not happen). */
    ROUND_FAILED
  };
  Status d_status = Status::UNKNOWN;
  /** On SUCCESS, the rounded value of each input variable. */
  std::vector<Integer> d_point;
  uint64_t d_pivots = 0;
};

/**
 * Runs the unit cube test on p: shifts every bound of variable v inwards by
 * p.shift(v), solves the shifted system, and rounds the solution to the
 * closest integer point. Requires all bounds of p to be non-strict and all
 * variables to be integral.
 */
CubeResult runUnitCubeTest(const CubeProblem& p, uint64_t pivotLimit);

/** An implied equality: variable d_var always equals d_value. */
struct ImpliedEquality
{
  size_t d_var;
  Rational d_value;
};

/** The outcome of the implied equality detection. */
struct ImpliedEqResult
{
  /**
   * Whether the detection finished, i.e., the strict system is satisfiable
   * once all equalities found are fixed. False if the pivot limit was
   * reached.
   */
  bool d_complete = false;
  /** The number of strict systems solved. */
  size_t d_rounds = 0;
  /**
   * The equalities found, besides those of variables whose bounds are
   * already equal in p.
   */
  std::vector<ImpliedEquality> d_equalities;
};

/**
 * Detects the implied equalities of p, which must be satisfiable over the
 * rationals: as long as the system with every bound made strict is
 * infeasible, every strict bound in the conflict holds with equality in all
 * solutions of p (Bromberger and Weidenbach, Lemma 6), so it is fixed as an
 * equality and the check is repeated. Variables whose bounds are already
 * equal keep non-strict bounds.
 */
ImpliedEqResult detectImpliedEqualities(const CubeProblem& p,
                                        uint64_t pivotLimit);

}  // namespace arith::linear
}  // namespace theory
}  // namespace cvc5::internal

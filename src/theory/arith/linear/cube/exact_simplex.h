/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * A standalone exact simplex for the unit cube test.
 *
 * This is the general simplex of Dutertre and de Moura (CAV 2006) over
 * delta-rationals, on its own Tableau. Unlike the simplex procedures of
 * TheoryArithPrivate it is not connected to the SAT context, the constraint
 * database or the output channel: bounds are tagged with ids chosen by the
 * caller, and an infeasible system is explained by the ids of the bounds in
 * the conflicting row. Nothing it derives can reach the SAT solver.
 */

#include "cvc5_private.h"

#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "theory/arith/delta_rational.h"
#include "theory/arith/linear/arithvar.h"
#include "theory/arith/linear/tableau.h"
#include "util/rational.h"

namespace cvc5::internal {
namespace theory {
namespace arith::linear {

/**
 * Decides the feasibility of a system of bounds on variables and on linear
 * combinations of variables (rows), over the rationals.
 *
 * A strict bound x < b is given as the delta-rational bound x <= b - delta,
 * where delta is a symbolic positive infinitesimal, as in TheoryArithPrivate.
 *
 * Bounds may be changed between calls to check(); the next call starts from
 * the current basis and assignment.
 */
class ExactSimplex
{
 public:
  enum class Status
  {
    SAT,
    INFEASIBLE,
    UNKNOWN
  };

  ExactSimplex();

  /** Adds a new structural variable, unbounded and with value 0. */
  ArithVar addVariable();
  /**
   * Adds a new variable s with the row s = sum_j coeffs[j].second * x_j, where
   * x_j = coeffs[j].first is a variable added before (structural or row), and
   * returns s. The coefficients must be nonzero and the variables distinct.
   */
  ArithVar addRow(const std::vector<std::pair<ArithVar, Rational>>& coeffs);

  /** Sets the lower bound of v to b, tagged with id. Replaces any previous. */
  void setLowerBound(ArithVar v, const DeltaRational& b, size_t id);
  /** Sets the upper bound of v to b, tagged with id. Replaces any previous. */
  void setUpperBound(ArithVar v, const DeltaRational& b, size_t id);
  /** Removes the lower bound of v, if any. */
  void removeLowerBound(ArithVar v);
  /** Removes the upper bound of v, if any. */
  void removeUpperBound(ArithVar v);

  /**
   * Searches for an assignment satisfying all bounds. Returns UNKNOWN if this
   * needs more than pivotLimit pivots.
   */
  Status check(uint64_t pivotLimit);

  /** The number of variables (structural and row). */
  size_t getNumVariables() const { return d_values.size(); }
  /** After SAT: the delta-rational value of v. */
  const DeltaRational& getAssignment(ArithVar v) const { return d_values[v]; }
  /**
   * After SAT: a positive rational delta0 such that substituting any delta in
   * (0, delta0] in the assignment satisfies all bounds. It is at most 1.
   */
  Rational computeDelta() const;
  /** After SAT: the value of v with delta substituted by delta. */
  Rational getValue(ArithVar v, const Rational& delta) const;
  /**
   * After INFEASIBLE: the ids of the bounds in the conflict. These are the
   * bounds of a row and of the variables on it, each with a nonzero Farkas
   * multiplier, whose conjunction is infeasible.
   */
  const std::vector<size_t>& getExplanation() const { return d_explanation; }
  /** The number of pivots done by the last call to check(). */
  uint64_t getPivots() const { return d_pivots; }

 private:
  /** A bound and the id the caller tagged it with. */
  struct Bound
  {
    DeltaRational d_value;
    size_t d_id;
  };

  bool belowLower(ArithVar v) const;
  bool aboveUpper(ArithVar v) const;
  /** Whether nonbasic v can be increased (resp. decreased) by some amount. */
  bool canIncrease(ArithVar v) const;
  bool canDecrease(ArithVar v) const;
  /** Sets the value of nonbasic x to v, updating the basic variables. */
  void update(ArithVar x, const DeltaRational& v);
  /**
   * Pivots basic b with nonbasic x, so that b gets value v. a is the
   * coefficient of x in the row of b.
   */
  void pivotAndUpdate(ArithVar b,
                      ArithVar x,
                      const Rational& a,
                      const DeltaRational& v);
  /** Records the conflict of row b, which is below its lower bound if low. */
  void explainRow(ArithVar b, bool low);
  /** Debug check: every row holds under the current assignment. */
  bool debugRowsHold() const;

  Tableau d_tableau;
  std::vector<DeltaRational> d_values;
  std::vector<std::optional<Bound>> d_lower;
  std::vector<std::optional<Bound>> d_upper;
  std::vector<size_t> d_explanation;
  uint64_t d_pivots;
};

}  // namespace arith::linear
}  // namespace theory
}  // namespace cvc5::internal

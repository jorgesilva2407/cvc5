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
 */

#include "theory/arith/linear/cube/exact_simplex.h"

#include "base/check.h"
#include "base/output.h"

namespace cvc5::internal {
namespace theory {
namespace arith::linear {

ExactSimplex::ExactSimplex() : d_pivots(0) {}

ArithVar ExactSimplex::addVariable()
{
  ArithVar v = d_values.size();
  d_tableau.increaseSize();
  d_values.emplace_back();
  d_lower.emplace_back();
  d_upper.emplace_back();
  return v;
}

ArithVar ExactSimplex::addRow(
    const std::vector<std::pair<ArithVar, Rational>>& coeffs)
{
  std::vector<Rational> cs;
  std::vector<ArithVar> vs;
  DeltaRational value;
  for (const auto& [x, a] : coeffs)
  {
    Assert(x < d_values.size());
    Assert(!a.isZero());
    cs.push_back(a);
    vs.push_back(x);
    value += d_values[x] * a;
  }
  ArithVar s = addVariable();
  // The tableau substitutes the rows of the variables that are basic.
  d_tableau.addRow(s, cs, vs);
  d_values[s] = value;
  Assert(debugRowsHold());
  return s;
}

void ExactSimplex::setLowerBound(ArithVar v, const DeltaRational& b, size_t id)
{
  Assert(v < d_values.size());
  d_lower[v] = Bound{b, id};
}

void ExactSimplex::setUpperBound(ArithVar v, const DeltaRational& b, size_t id)
{
  Assert(v < d_values.size());
  d_upper[v] = Bound{b, id};
}

void ExactSimplex::removeLowerBound(ArithVar v) { d_lower[v].reset(); }

void ExactSimplex::removeUpperBound(ArithVar v) { d_upper[v].reset(); }

bool ExactSimplex::belowLower(ArithVar v) const
{
  return d_lower[v] && d_values[v] < d_lower[v]->d_value;
}

bool ExactSimplex::aboveUpper(ArithVar v) const
{
  return d_upper[v] && d_values[v] > d_upper[v]->d_value;
}

bool ExactSimplex::canIncrease(ArithVar v) const
{
  return !d_upper[v] || d_values[v] < d_upper[v]->d_value;
}

bool ExactSimplex::canDecrease(ArithVar v) const
{
  return !d_lower[v] || d_values[v] > d_lower[v]->d_value;
}

void ExactSimplex::update(ArithVar x, const DeltaRational& v)
{
  Assert(!d_tableau.isBasic(x));
  DeltaRational diff = v - d_values[x];
  for (Tableau::ColIterator it = d_tableau.colIterator(x); !it.atEnd(); ++it)
  {
    const Tableau::Entry& entry = *it;
    ArithVar basic = d_tableau.rowIndexToBasic(entry.getRowIndex());
    d_values[basic] += diff * entry.getCoefficient();
  }
  d_values[x] = v;
}

void ExactSimplex::pivotAndUpdate(ArithVar b,
                                  ArithVar x,
                                  const Rational& a,
                                  const DeltaRational& v)
{
  Assert(d_tableau.isBasic(b));
  Assert(!d_tableau.isBasic(x));
  // Moving x by theta moves b by a * theta.
  DeltaRational theta = (v - d_values[b]) / a;
  RowIndex rowB = d_tableau.basicToRowIndex(b);
  for (Tableau::ColIterator it = d_tableau.colIterator(x); !it.atEnd(); ++it)
  {
    const Tableau::Entry& entry = *it;
    if (entry.getRowIndex() != rowB)
    {
      ArithVar basic = d_tableau.rowIndexToBasic(entry.getRowIndex());
      d_values[basic] += theta * entry.getCoefficient();
    }
  }
  d_values[b] = v;
  d_values[x] += theta;

  NoEffectCCCB cb;
  d_tableau.pivot(b, x, cb);
  ++d_pivots;
}

void ExactSimplex::explainRow(ArithVar b, bool low)
{
  // The row is b = sum_j a_j x_j. If b is below its lower bound, every x_j
  // with a_j > 0 is at its upper bound and every x_j with a_j < 0 is at its
  // lower bound, and symmetrically if b is above its upper bound.
  d_explanation.clear();
  d_explanation.push_back(low ? d_lower[b]->d_id : d_upper[b]->d_id);
  for (Tableau::RowIterator it = d_tableau.basicRowIterator(b); !it.atEnd();
       ++it)
  {
    const Tableau::Entry& entry = *it;
    ArithVar x = entry.getColVar();
    if (x == b)
    {
      continue;
    }
    bool useUpper = (entry.getCoefficient().sgn() > 0) == low;
    const std::optional<Bound>& bound = useUpper ? d_upper[x] : d_lower[x];
    Assert(bound && d_values[x] == bound->d_value);
    d_explanation.push_back(bound->d_id);
  }
}

ExactSimplex::Status ExactSimplex::check(uint64_t pivotLimit)
{
  d_explanation.clear();
  d_pivots = 0;

  // Nonbasic variables are kept within their bounds, which is impossible if
  // the bounds cross.
  for (ArithVar v = 0, n = d_values.size(); v < n; ++v)
  {
    if (d_lower[v] && d_upper[v] && d_lower[v]->d_value > d_upper[v]->d_value)
    {
      d_explanation = {d_lower[v]->d_id, d_upper[v]->d_id};
      Trace("arith::cube::simplex")
          << "simplex: crossing bounds on " << v << std::endl;
      return Status::INFEASIBLE;
    }
  }
  for (ArithVar v = 0, n = d_values.size(); v < n; ++v)
  {
    if (!d_tableau.isBasic(v))
    {
      if (belowLower(v))
      {
        update(v, d_lower[v]->d_value);
      }
      else if (aboveUpper(v))
      {
        update(v, d_upper[v]->d_value);
      }
    }
  }

  // Bland's rule: always the smallest violated basic variable and the
  // smallest suitable nonbasic one. This guarantees termination.
  while (true)
  {
    Assert(debugRowsHold());
    ArithVar b = ARITHVAR_SENTINEL;
    for (ArithVar v = 0, n = d_values.size(); v < n; ++v)
    {
      if (d_tableau.isBasic(v) && (belowLower(v) || aboveUpper(v)))
      {
        b = v;
        break;
      }
    }
    if (b == ARITHVAR_SENTINEL)
    {
      Trace("arith::cube::simplex")
          << "simplex: sat after " << d_pivots << " pivots" << std::endl;
      return Status::SAT;
    }
    if (d_pivots >= pivotLimit)
    {
      Trace("arith::cube::simplex")
          << "simplex: pivot limit reached" << std::endl;
      return Status::UNKNOWN;
    }

    bool low = belowLower(b);
    ArithVar entering = ARITHVAR_SENTINEL;
    Rational enteringCoeff;
    for (Tableau::RowIterator it = d_tableau.basicRowIterator(b); !it.atEnd();
         ++it)
    {
      const Tableau::Entry& entry = *it;
      ArithVar x = entry.getColVar();
      if (x == b || (entering != ARITHVAR_SENTINEL && x > entering))
      {
        continue;
      }
      // To increase b, increase x if a > 0 and decrease it if a < 0.
      bool increase = (entry.getCoefficient().sgn() > 0) == low;
      if (increase ? canIncrease(x) : canDecrease(x))
      {
        entering = x;
        enteringCoeff = entry.getCoefficient();
      }
    }
    if (entering == ARITHVAR_SENTINEL)
    {
      explainRow(b, low);
      Trace("arith::cube::simplex") << "simplex: infeasible after " << d_pivots
                                    << " pivots" << std::endl;
      return Status::INFEASIBLE;
    }
    pivotAndUpdate(b,
                   entering,
                   enteringCoeff,
                   low ? d_lower[b]->d_value : d_upper[b]->d_value);
  }
}

Rational ExactSimplex::computeDelta() const
{
  // A bound l = c1 + k1 delta <= c2 + k2 delta = value holds
  // lexicographically. If c1 < c2 and k1 > k2 it only holds for
  // delta <= (c2 - c1) / (k1 - k2); otherwise it holds for every delta > 0.
  Rational delta(1);
  auto limit = [&delta](const DeltaRational& lo, const DeltaRational& hi) {
    const Rational& c1 = lo.getNoninfinitesimalPart();
    const Rational& k1 = lo.getInfinitesimalPart();
    const Rational& c2 = hi.getNoninfinitesimalPart();
    const Rational& k2 = hi.getInfinitesimalPart();
    if (c1 < c2 && k1 > k2)
    {
      Rational max = (c2 - c1) / (k1 - k2);
      if (max < delta)
      {
        delta = max;
      }
    }
  };
  for (ArithVar v = 0, n = d_values.size(); v < n; ++v)
  {
    if (d_lower[v])
    {
      limit(d_lower[v]->d_value, d_values[v]);
    }
    if (d_upper[v])
    {
      limit(d_values[v], d_upper[v]->d_value);
    }
  }
  return delta;
}

Rational ExactSimplex::getValue(ArithVar v, const Rational& delta) const
{
  return d_values[v].substituteDelta(delta);
}

bool ExactSimplex::debugRowsHold() const
{
  for (Tableau::BasicIterator bi = d_tableau.beginBasic(),
                              bend = d_tableau.endBasic();
       bi != bend;
       ++bi)
  {
    ArithVar b = *bi;
    DeltaRational sum;
    for (Tableau::RowIterator it = d_tableau.basicRowIterator(b); !it.atEnd();
         ++it)
    {
      const Tableau::Entry& entry = *it;
      if (entry.getColVar() != b)
      {
        sum += d_values[entry.getColVar()] * entry.getCoefficient();
      }
    }
    if (sum != d_values[b])
    {
      return false;
    }
  }
  return true;
}

}  // namespace arith::linear
}  // namespace theory
}  // namespace cvc5::internal

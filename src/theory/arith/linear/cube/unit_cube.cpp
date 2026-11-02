/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The unit cube test and the detection of implied equalities.
 */

#include "theory/arith/linear/cube/unit_cube.h"

#include "base/check.h"
#include "base/output.h"
#include "theory/arith/linear/cube/exact_simplex.h"

namespace cvc5::internal {
namespace theory {
namespace arith::linear {

size_t CubeProblem::addInput()
{
  Assert(d_rows.empty()) << "inputs must be added before rows";
  d_lower.emplace_back();
  d_upper.emplace_back();
  return d_numInputs++;
}

size_t CubeProblem::addRow(std::vector<std::pair<size_t, Rational>> coeffs)
{
  d_rows.push_back(std::move(coeffs));
  d_lower.emplace_back();
  d_upper.emplace_back();
  return numVariables() - 1;
}

Rational CubeProblem::shift(size_t v) const
{
  if (v < d_numInputs)
  {
    return Rational(1, 2);
  }
  Rational norm;
  for (const auto& entry : d_rows[v - d_numInputs])
  {
    norm += entry.second.abs();
  }
  return norm / Rational(2);
}

std::vector<Rational> CubeProblem::evaluate(
    const std::vector<Integer>& inputs) const
{
  Assert(inputs.size() == d_numInputs);
  std::vector<Rational> values;
  for (const Integer& i : inputs)
  {
    values.emplace_back(i);
  }
  for (const auto& row : d_rows)
  {
    Rational sum;
    for (const auto& [x, a] : row)
    {
      sum += a * values[x];
    }
    values.push_back(sum);
  }
  return values;
}

bool CubeProblem::hasEquality() const
{
  for (size_t v = 0, n = numVariables(); v < n; ++v)
  {
    if (d_lower[v] && d_upper[v] && *d_lower[v] == *d_upper[v])
    {
      return true;
    }
  }
  return false;
}

namespace {

/**
 * Adds the variables of p to s. Rows without bounds are left out. Returns the
 * variable of s for each variable of p, or ARITHVAR_SENTINEL if left out.
 */
std::vector<ArithVar> addVariables(const CubeProblem& p, ExactSimplex& s)
{
  std::vector<ArithVar> vars;
  for (size_t j = 0; j < p.d_numInputs; ++j)
  {
    vars.push_back(s.addVariable());
  }
  for (size_t i = 0, m = p.d_rows.size(); i < m; ++i)
  {
    size_t v = p.d_numInputs + i;
    if (!p.d_lower[v] && !p.d_upper[v])
    {
      vars.push_back(ARITHVAR_SENTINEL);
      continue;
    }
    std::vector<std::pair<ArithVar, Rational>> coeffs;
    for (const auto& [x, a] : p.d_rows[i])
    {
      coeffs.emplace_back(vars[x], a);
    }
    vars.push_back(s.addRow(coeffs));
  }
  return vars;
}

/** The id of the lower (resp. upper) bound of v in the simplex. */
size_t lowerId(size_t v) { return 2 * v; }
size_t upperId(size_t v) { return 2 * v + 1; }

}  // namespace

CubeResult runUnitCubeTest(const CubeProblem& p, uint64_t pivotLimit)
{
  CubeResult res;
  ExactSimplex s;
  std::vector<ArithVar> vars = addVariables(p, s);
  for (size_t v = 0, n = p.numVariables(); v < n; ++v)
  {
    if (!p.d_lower[v] && !p.d_upper[v])
    {
      continue;
    }
    DeltaRational shift(p.shift(v));
    std::optional<DeltaRational> lb, ub;
    if (p.d_lower[v])
    {
      Assert(p.d_lower[v]->infinitesimalIsZero());
      lb = *p.d_lower[v] + shift;
      s.setLowerBound(vars[v], *lb, lowerId(v));
    }
    if (p.d_upper[v])
    {
      Assert(p.d_upper[v]->infinitesimalIsZero());
      ub = *p.d_upper[v] - shift;
      s.setUpperBound(vars[v], *ub, upperId(v));
    }
    if (lb && ub && *lb > *ub)
    {
      Trace("arith::cube") << "cube: no room for variable " << v << std::endl;
      res.d_status = CubeResult::Status::NO_ROOM;
      return res;
    }
  }

  ExactSimplex::Status st = s.check(pivotLimit);
  res.d_pivots = s.getPivots();
  if (st != ExactSimplex::Status::SAT)
  {
    res.d_status = st == ExactSimplex::Status::INFEASIBLE
                       ? CubeResult::Status::SHIFTED_INFEASIBLE
                       : CubeResult::Status::UNKNOWN;
    return res;
  }

  // Round the center z to the closest integer point floor(z + 1/2). All
  // bounds are non-strict, so the assignment has no infinitesimal part.
  Rational half(1, 2);
  for (size_t j = 0; j < p.d_numInputs; ++j)
  {
    const DeltaRational& z = s.getAssignment(vars[j]);
    Assert(z.infinitesimalIsZero());
    res.d_point.push_back((z.getNoninfinitesimalPart() + half).floor());
    Trace("arith::cube") << "cube: x" << j << " " << z << " -> "
                         << res.d_point.back() << std::endl;
  }

  // By the choice of the shifts, the rounded point satisfies every bound.
  std::vector<Rational> values = p.evaluate(res.d_point);
  for (size_t v = 0, n = p.numVariables(); v < n; ++v)
  {
    if ((p.d_lower[v] && DeltaRational(values[v]) < *p.d_lower[v])
        || (p.d_upper[v] && DeltaRational(values[v]) > *p.d_upper[v]))
    {
      Assert(false) << "cube: rounded point violates a bound of variable "
                    << v;
      res.d_status = CubeResult::Status::ROUND_FAILED;
      return res;
    }
  }
  res.d_status = CubeResult::Status::SUCCESS;
  return res;
}

ImpliedEqResult detectImpliedEqualities(const CubeProblem& p,
                                        uint64_t pivotLimit)
{
  ImpliedEqResult res;
  ExactSimplex s;
  std::vector<ArithVar> vars = addVariables(p, s);
  // Whether the bound with each id is strict.
  std::vector<bool> strict(2 * p.numVariables(), false);
  for (size_t v = 0, n = p.numVariables(); v < n; ++v)
  {
    const std::optional<DeltaRational>& lb = p.d_lower[v];
    const std::optional<DeltaRational>& ub = p.d_upper[v];
    if (lb && ub && *lb == *ub)
    {
      // already an equality
      s.setLowerBound(vars[v], *lb, lowerId(v));
      s.setUpperBound(vars[v], *ub, upperId(v));
      continue;
    }
    if (lb)
    {
      Assert(lb->infinitesimalIsZero());
      s.setLowerBound(
          vars[v], DeltaRational(lb->getNoninfinitesimalPart(), 1), lowerId(v));
      strict[lowerId(v)] = true;
    }
    if (ub)
    {
      Assert(ub->infinitesimalIsZero());
      s.setUpperBound(vars[v],
                      DeltaRational(ub->getNoninfinitesimalPart(), -1),
                      upperId(v));
      strict[upperId(v)] = true;
    }
  }

  while (true)
  {
    ++res.d_rounds;
    ExactSimplex::Status st = s.check(pivotLimit);
    if (st == ExactSimplex::Status::SAT)
    {
      res.d_complete = true;
      return res;
    }
    if (st == ExactSimplex::Status::UNKNOWN)
    {
      return res;
    }
    // Every strict bound in the conflict holds with equality in all solutions
    // of p. Fix them as equalities and try again.
    bool found = false;
    for (size_t id : s.getExplanation())
    {
      if (!strict[id])
      {
        continue;
      }
      size_t v = id / 2;
      const DeltaRational& b = (id == lowerId(v)) ? *p.d_lower[v]
                                                  : *p.d_upper[v];
      s.setLowerBound(vars[v], b, lowerId(v));
      s.setUpperBound(vars[v], b, upperId(v));
      strict[lowerId(v)] = false;
      strict[upperId(v)] = false;
      res.d_equalities.push_back({v, b.getNoninfinitesimalPart()});
      Trace("arith::cube") << "cube: implied equality: variable " << v
                           << " = " << b << std::endl;
      found = true;
    }
    // Otherwise the non-strict system is infeasible, contradicting the
    // precondition.
    Assert(found) << "cube: implied equality detection on an infeasible system";
    if (!found)
    {
      return res;
    }
  }
}

}  // namespace arith::linear
}  // namespace theory
}  // namespace cvc5::internal

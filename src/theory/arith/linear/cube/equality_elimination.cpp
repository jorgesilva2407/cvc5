/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Elimination of the equalities of a CubeProblem with Griggio's method.
 *
 * The structure follows DioSolver::processEquations: equalities wait in a
 * queue; the one with the coefficient of least absolute value is solved
 * directly if that coefficient is 1 (solveIndex), otherwise an equality with
 * a coefficient of 1 is derived if the column gcd allows it
 * (impliedGcdOfOne), and otherwise it is decomposed with a fresh variable
 * (decomposeIndex). Each solved variable becomes a substitution that is
 * applied to the queue (subAndReduceCurrentFByIndex).
 */

#include "theory/arith/linear/cube/equality_elimination.h"

#include <deque>
#include <map>
#include <optional>

#include "base/check.h"
#include "base/output.h"

namespace cvc5::internal {
namespace theory {
namespace arith::linear {

Integer AffineExpr::evaluate(const std::vector<Integer>& y) const
{
  Integer sum = d_constant;
  for (const auto& [j, a] : d_coeffs)
  {
    Assert(j < y.size());
    sum += a * y[j];
  }
  return sum;
}

namespace {

/** The equation sum_j a_j x_j + c = 0 over the integers. */
struct Equation
{
  std::map<size_t, Integer> d_coeffs;
  Integer d_constant;

  bool triviallySat() const
  {
    return d_coeffs.empty() && d_constant.isZero();
  }
  bool triviallyUnsat() const
  {
    return d_coeffs.empty() && !d_constant.isZero();
  }
  /** The coefficient of x, or zero. */
  Integer coefficient(size_t x) const
  {
    auto it = d_coeffs.find(x);
    return it == d_coeffs.end() ? Integer(0) : it->second;
  }
  /** The variable with the coefficient of least absolute value. */
  size_t minimalVariable() const
  {
    Assert(!d_coeffs.empty());
    auto best = d_coeffs.begin();
    for (auto it = d_coeffs.begin(); it != d_coeffs.end(); ++it)
    {
      if (it->second.abs() < best->second.abs())
      {
        best = it;
      }
    }
    return best->first;
  }
  /** this += k * e */
  void addMultiple(const Equation& e, const Integer& k)
  {
    for (const auto& [x, a] : e.d_coeffs)
    {
      Integer c = coefficient(x) + k * a;
      if (c.isZero())
      {
        d_coeffs.erase(x);
      }
      else
      {
        d_coeffs[x] = c;
      }
    }
    d_constant += k * e.d_constant;
  }
  /** s * this + t * e */
  Equation combine(const Integer& s, const Equation& e, const Integer& t) const
  {
    Equation res;
    res.addMultiple(*this, s);
    res.addMultiple(e, t);
    return res;
  }
};

/**
 * A substitution x_d_var = sum_{j != d_var} a_j x_j + c, stored as in
 * DioSolver as the equation d_eq, whose coefficient of x_d_var is -1.
 */
struct Substitution
{
  size_t d_var;
  Equation d_eq;
};

class Eliminator
{
 public:
  Eliminator(size_t numVars, uint64_t stepLimit)
      : d_numVars(numVars), d_stepLimit(stepLimit)
  {
  }

  void pushInputConstraint(Equation e)
  {
    if (!d_infeasible)
    {
      enqueue(std::move(e));
    }
  }

  /** Solves all equations. Returns the status of the elimination. */
  EliminationResult::Status processEquations();

  size_t numVariables() const { return d_numVars; }
  size_t numFresh() const { return d_fresh; }
  /**
   * After processEquations, each eliminated variable as an expression over
   * the variables that are not eliminated.
   */
  std::map<size_t, Equation> solvedForm() const;

 private:
  /** Reduces e by its gcd and adds it to the queue if it is not trivial. */
  void enqueue(Equation e);
  /**
   * Divides e by the gcd of its coefficients. Returns false if the gcd does
   * not divide the constant, i.e., e has no integer solution.
   */
  bool reduceByGCD(Equation& e) const;
  void moveMinimumByAbsToQueueFront();
  /** A variable whose coefficients in the queue have gcd 1, if any. */
  std::optional<size_t> columnGcdIsOne() const;
  /** An equation with a coefficient of 1, derived from the queue, if any. */
  std::optional<Equation> impliedGcdOfOne() const;
  /** Solves e, which has a coefficient of absolute value 1 for x. */
  size_t solveIndex(Equation e, size_t x);
  /**
   * Decomposes e with a fresh variable. Returns the substitution and sets
   * next to the equation that remains.
   */
  size_t decomposeIndex(const Equation& e, Equation& next);
  /** Applies the substitution to the queue. */
  void subAndReduceCurrentFByIndex(size_t subIndex);

  /** The number of variables, including the fresh ones. */
  size_t d_numVars;
  size_t d_fresh = 0;
  uint64_t d_steps = 0;
  uint64_t d_stepLimit;
  bool d_infeasible = false;
  std::deque<Equation> d_currentF;
  std::vector<Substitution> d_subs;
};

bool Eliminator::reduceByGCD(Equation& e) const
{
  Assert(!e.d_coeffs.empty());
  Integer g;
  for (const auto& entry : e.d_coeffs)
  {
    g = g.gcd(entry.second);
  }
  Assert(g >= 1);
  if (!g.divides(e.d_constant))
  {
    return false;
  }
  if (g > 1)
  {
    for (auto& entry : e.d_coeffs)
    {
      entry.second = entry.second.exactQuotient(g);
    }
    e.d_constant = e.d_constant.exactQuotient(g);
  }
  return true;
}

void Eliminator::enqueue(Equation e)
{
  if (e.triviallySat())
  {
    return;
  }
  if (e.triviallyUnsat() || !reduceByGCD(e))
  {
    d_infeasible = true;
    return;
  }
  d_currentF.push_back(std::move(e));
}

void Eliminator::moveMinimumByAbsToQueueFront()
{
  Assert(!d_currentF.empty());
  size_t best = 0;
  Integer bestAbs = d_currentF[0].coefficient(d_currentF[0].minimalVariable()).abs();
  for (size_t i = 1, n = d_currentF.size(); i < n; ++i)
  {
    Integer a = d_currentF[i].coefficient(d_currentF[i].minimalVariable()).abs();
    if (a < bestAbs)
    {
      best = i;
      bestAbs = a;
    }
  }
  std::swap(d_currentF[best], d_currentF.front());
}

std::optional<size_t> Eliminator::columnGcdIsOne() const
{
  std::map<size_t, Integer> gcds;
  for (const Equation& e : d_currentF)
  {
    for (const auto& [x, a] : e.d_coeffs)
    {
      auto it = gcds.find(x);
      if (it == gcds.end())
      {
        // Called only if no coefficient has absolute value 1.
        Assert(a.abs() > 1);
        gcds.emplace(x, a.abs());
      }
      else
      {
        it->second = it->second.gcd(a);
        if (it->second.isOne())
        {
          return x;
        }
      }
    }
  }
  return std::nullopt;
}

std::optional<Equation> Eliminator::impliedGcdOfOne() const
{
  std::optional<size_t> x = columnGcdIsOne();
  if (!x)
  {
    return std::nullopt;
  }
  // Combine the equations containing x with the extended gcd of their
  // coefficients of x until it is 1.
  auto it = d_currentF.begin(), end = d_currentF.end();
  while (it->coefficient(*x).isZero())
  {
    ++it;
    Assert(it != end);
  }
  Equation current = *it;
  Integer currentCoeff = current.coefficient(*x);
  Integer currentGcd = currentCoeff.abs();
  for (++it; it != end; ++it)
  {
    Integer inQueueCoeff = it->coefficient(*x);
    if (inQueueCoeff.isZero())
    {
      continue;
    }
    // g = s * currentCoeff + t * inQueueCoeff
    Integer g, s, t;
    Integer::extendedGcd(g, s, t, currentCoeff, inQueueCoeff);
    Assert(g <= currentGcd);
    if (g < currentGcd)
    {
      if (s.sgn() == 0)
      {
        Assert(inQueueCoeff.divides(currentGcd));
        current = *it;
        currentCoeff = inQueueCoeff;
        currentGcd = inQueueCoeff.abs();
      }
      else
      {
        current = current.combine(s, *it, t);
        Assert(current.coefficient(*x) == g);
        currentCoeff = g;
        currentGcd = g;
        if (currentGcd.isOne())
        {
          return current;
        }
      }
    }
  }
  // Not reachable, since the gcd of the column is 1.
  Unreachable();
}

size_t Eliminator::solveIndex(Equation e, size_t x)
{
  Integer a = e.coefficient(x);
  Assert(a.abs().isOne());
  if (a.sgn() > 0)
  {
    // normalize the coefficient of x to -1
    Equation neg;
    neg.addMultiple(e, Integer(-1));
    e = std::move(neg);
  }
  Trace("arith::cube::elim") << "elim: solve x" << x << std::endl;
  d_subs.push_back({x, std::move(e)});
  return d_subs.size() - 1;
}

size_t Eliminator::decomposeIndex(const Equation& e, Equation& next)
{
  size_t x = e.minimalVariable();
  Integer a = e.coefficient(x);
  Assert(a.abs() > 1);
  // a_j = a q_j + r_j and c = a q_c + r_c. With the fresh variable sigma,
  // x = sigma - sum_{j != x} q_j x_j - q_c turns e into
  // a sigma + sum_j r_j x_j + r_c = 0.
  size_t sigma = d_numVars++;
  ++d_fresh;
  Equation sub, rest;
  sub.d_coeffs[sigma] = Integer(1);
  for (const auto& [j, aj] : e.d_coeffs)
  {
    Integer q, r;
    Integer::floorQR(q, r, aj, a);
    if (!q.isZero())
    {
      sub.d_coeffs[j] = -q;
    }
    if (!r.isZero())
    {
      rest.d_coeffs[j] = r;
    }
  }
  Integer qc, rc;
  Integer::floorQR(qc, rc, e.d_constant, a);
  sub.d_constant = -qc;
  rest.d_constant = rc;
  rest.d_coeffs[sigma] = a;
  Assert(sub.coefficient(x) == Integer(-1));
  Assert(rest.coefficient(x).isZero());
  Trace("arith::cube::elim") << "elim: decompose x" << x << " with x" << sigma
                             << std::endl;
  next = std::move(rest);
  d_subs.push_back({x, std::move(sub)});
  return d_subs.size() - 1;
}

void Eliminator::subAndReduceCurrentFByIndex(size_t subIndex)
{
  const Substitution& s = d_subs[subIndex];
  std::deque<Equation> queue;
  for (Equation& e : d_currentF)
  {
    Integer a = e.coefficient(s.d_var);
    if (!a.isZero())
    {
      e.addMultiple(s.d_eq, a);
      Assert(e.coefficient(s.d_var).isZero());
      if (e.triviallySat())
      {
        continue;
      }
      if (e.triviallyUnsat() || !reduceByGCD(e))
      {
        d_infeasible = true;
        return;
      }
    }
    queue.push_back(std::move(e));
  }
  d_currentF = std::move(queue);
}

EliminationResult::Status Eliminator::processEquations()
{
  while (!d_currentF.empty() && !d_infeasible)
  {
    if (++d_steps > d_stepLimit)
    {
      return EliminationResult::Status::UNKNOWN;
    }
    moveMinimumByAbsToQueueFront();
    const Equation& minimum = d_currentF.front();
    size_t x = minimum.minimalVariable();
    std::optional<Equation> next;
    size_t subIndex;
    if (minimum.coefficient(x).abs().isOne())
    {
      Equation e = std::move(d_currentF.front());
      d_currentF.pop_front();
      subIndex = solveIndex(std::move(e), x);
    }
    else if (std::optional<Equation> implied = impliedGcdOfOne())
    {
      size_t y = implied->minimalVariable();
      subIndex = solveIndex(std::move(*implied), y);
    }
    else
    {
      Equation e = std::move(d_currentF.front());
      d_currentF.pop_front();
      next.emplace();
      subIndex = decomposeIndex(e, *next);
    }
    subAndReduceCurrentFByIndex(subIndex);
    if (next && !d_infeasible)
    {
      if (next->triviallyUnsat())
      {
        d_infeasible = true;
      }
      else if (!next->triviallySat())
      {
        d_currentF.push_back(std::move(*next));
      }
    }
  }
  return d_infeasible ? EliminationResult::Status::INFEASIBLE
                      : EliminationResult::Status::REDUCED;
}

std::map<size_t, Equation> Eliminator::solvedForm() const
{
  // A substitution only contains variables that are free or solved later, so
  // the substitutions are resolved from the last to the first.
  std::map<size_t, Equation> solved;
  for (auto it = d_subs.rbegin(); it != d_subs.rend(); ++it)
  {
    Equation value;
    for (const auto& [j, a] : it->d_eq.d_coeffs)
    {
      if (j == it->d_var)
      {
        continue;
      }
      auto sj = solved.find(j);
      if (sj != solved.end())
      {
        value.addMultiple(sj->second, a);
      }
      else
      {
        Equation var;
        var.d_coeffs[j] = Integer(1);
        value.addMultiple(var, a);
      }
    }
    value.d_constant += it->d_eq.d_constant;
    Assert(solved.find(it->d_var) == solved.end());
    solved.emplace(it->d_var, std::move(value));
  }
  return solved;
}

/** The equation sum_j a_j x_j - b = 0, scaled to integer coefficients. */
Equation integralEquation(const std::vector<std::pair<size_t, Rational>>& row,
                          const Rational& b)
{
  Integer l = b.getDenominator();
  for (const auto& entry : row)
  {
    l = l.lcm(entry.second.getDenominator());
  }
  Equation e;
  for (const auto& [x, a] : row)
  {
    Rational c = a * Rational(l);
    Assert(c.isIntegral());
    e.d_coeffs[x] += c.getNumerator();
    if (e.d_coeffs[x].isZero())
    {
      e.d_coeffs.erase(x);
    }
  }
  Rational c = b * Rational(l);
  Assert(c.isIntegral());
  e.d_constant = -c.getNumerator();
  return e;
}

/**
 * Adds the bounds of p's variable v, whose value is constant + sum, to the
 * reduced problem q. Returns false if they are violated by a constant value.
 */
bool addBounds(const CubeProblem& p,
               size_t v,
               const Rational& constant,
               const std::vector<std::pair<size_t, Rational>>& sum,
               CubeProblem& q)
{
  const std::optional<DeltaRational>& lb = p.d_lower[v];
  const std::optional<DeltaRational>& ub = p.d_upper[v];
  if (!lb && !ub)
  {
    return true;
  }
  DeltaRational c(constant);
  if (sum.empty())
  {
    return !(lb && c < *lb) && !(ub && c > *ub);
  }
  size_t w;
  if (sum.size() == 1 && sum[0].second.isOne() && sum[0].first < q.d_numInputs
      && !q.d_lower[sum[0].first] && !q.d_upper[sum[0].first])
  {
    // The bounds are put on the variable itself instead of a row.
    w = sum[0].first;
  }
  else
  {
    w = q.addRow(sum);
  }
  if (lb)
  {
    q.d_lower[w] = *lb - c;
  }
  if (ub)
  {
    q.d_upper[w] = *ub - c;
  }
  return true;
}

}  // namespace

EliminationResult eliminateEqualities(const CubeProblem& p,
                                      uint64_t stepLimit)
{
  EliminationResult res;
  Eliminator elim(p.d_numInputs, stepLimit);
  for (size_t v = 0, n = p.numVariables(); v < n; ++v)
  {
    const std::optional<DeltaRational>& lb = p.d_lower[v];
    const std::optional<DeltaRational>& ub = p.d_upper[v];
    if (!lb || !ub || *lb != *ub)
    {
      continue;
    }
    Assert(lb->infinitesimalIsZero());
    ++res.d_equalities;
    const Rational& b = lb->getNoninfinitesimalPart();
    if (v < p.d_numInputs)
    {
      elim.pushInputConstraint(integralEquation({{v, Rational(1)}}, b));
    }
    else
    {
      elim.pushInputConstraint(
          integralEquation(p.d_rows[v - p.d_numInputs], b));
    }
  }
  res.d_status = elim.processEquations();
  res.d_fresh = elim.numFresh();
  if (res.d_status != EliminationResult::Status::REDUCED)
  {
    Trace("arith::cube::elim") << "elim: gave up" << std::endl;
    return res;
  }

  // The variables that are not eliminated are the inputs of the reduced
  // problem.
  std::map<size_t, Equation> solved = elim.solvedForm();
  std::vector<size_t> index(elim.numVariables());
  for (size_t x = 0, n = elim.numVariables(); x < n; ++x)
  {
    if (solved.find(x) == solved.end())
    {
      index[x] = res.d_reduced.addInput();
    }
  }
  for (size_t i = 0; i < p.d_numInputs; ++i)
  {
    AffineExpr e;
    auto it = solved.find(i);
    if (it == solved.end())
    {
      e.d_coeffs.emplace_back(index[i], Integer(1));
    }
    else
    {
      e.d_constant = it->second.d_constant;
      for (const auto& [x, a] : it->second.d_coeffs)
      {
        Assert(solved.find(x) == solved.end());
        e.d_coeffs.emplace_back(index[x], a);
      }
    }
    res.d_map.push_back(std::move(e));
  }

  // Substitute the map into every bound of p.
  for (size_t v = 0, n = p.numVariables(); v < n; ++v)
  {
    Rational constant;
    std::map<size_t, Rational> sum;
    auto addTerm = [&](size_t i, const Rational& a) {
      const AffineExpr& e = res.d_map[i];
      constant += a * Rational(e.d_constant);
      for (const auto& [y, c] : e.d_coeffs)
      {
        sum[y] += a * Rational(c);
      }
    };
    if (v < p.d_numInputs)
    {
      addTerm(v, Rational(1));
    }
    else
    {
      for (const auto& [i, a] : p.d_rows[v - p.d_numInputs])
      {
        addTerm(i, a);
      }
    }
    std::vector<std::pair<size_t, Rational>> terms;
    for (const auto& [y, c] : sum)
    {
      if (!c.isZero())
      {
        terms.emplace_back(y, c);
      }
    }
    if (!addBounds(p, v, constant, terms, res.d_reduced))
    {
      Trace("arith::cube::elim")
          << "elim: the equalities violate a bound of variable " << v
          << std::endl;
      res.d_status = EliminationResult::Status::INFEASIBLE;
      return res;
    }
  }
  Trace("arith::cube::elim") << "elim: " << res.d_equalities
                             << " equalities eliminated, " << res.d_fresh
                             << " fresh variables, "
                             << res.d_reduced.d_numInputs << " left"
                             << std::endl;
  return res;
}

std::vector<AffineExpr> composeMaps(const std::vector<AffineExpr>& map,
                                    const std::vector<AffineExpr>& inner)
{
  std::vector<AffineExpr> res;
  for (const AffineExpr& e : map)
  {
    AffineExpr c;
    c.d_constant = e.d_constant;
    std::map<size_t, Integer> sum;
    for (const auto& [y, a] : e.d_coeffs)
    {
      Assert(y < inner.size());
      c.d_constant += a * inner[y].d_constant;
      for (const auto& [z, b] : inner[y].d_coeffs)
      {
        sum[z] += a * b;
      }
    }
    for (const auto& [z, b] : sum)
    {
      if (!b.isZero())
      {
        c.d_coeffs.emplace_back(z, b);
      }
    }
    res.push_back(std::move(c));
  }
  return res;
}

std::vector<Integer> applyMap(const std::vector<AffineExpr>& map,
                              const std::vector<Integer>& y)
{
  std::vector<Integer> x;
  for (const AffineExpr& e : map)
  {
    x.push_back(e.evaluate(y));
  }
  return x;
}

}  // namespace arith::linear
}  // namespace theory
}  // namespace cvc5::internal

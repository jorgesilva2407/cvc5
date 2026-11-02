/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * White box testing of the exact simplex used by the unit cube test.
 */

#include <algorithm>
#include <random>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "test.h"
#include "theory/arith/delta_rational.h"
#include "theory/arith/linear/cube/exact_simplex.h"
#include "util/rational.h"

namespace cvc5::internal {

using namespace theory::arith;
using namespace theory::arith::linear;

namespace test {

using Row = std::vector<std::pair<ArithVar, Rational>>;

class TestTheoryWhiteArithExactSimplex : public TestInternal
{
 protected:
  /** x < b, as the bound x <= b - delta. */
  static DeltaRational below(int64_t b) { return DeltaRational(b, -1); }
  /** x > b, as the bound x >= b + delta. */
  static DeltaRational above(int64_t b) { return DeltaRational(b, 1); }

  static std::set<size_t> explanation(const ExactSimplex& s)
  {
    const std::vector<size_t>& e = s.getExplanation();
    return std::set<size_t>(e.begin(), e.end());
  }
};

TEST_F(TestTheoryWhiteArithExactSimplex, sat_bounded)
{
  // x >= 0, y >= 0, x + y <= 4, x - y >= 1
  ExactSimplex s;
  ArithVar x = s.addVariable();
  ArithVar y = s.addVariable();
  ArithVar sum = s.addRow({{x, Rational(1)}, {y, Rational(1)}});
  ArithVar diff = s.addRow({{x, Rational(1)}, {y, Rational(-1)}});
  s.setLowerBound(x, Rational(0), 0);
  s.setLowerBound(y, Rational(0), 1);
  s.setUpperBound(sum, Rational(4), 2);
  s.setLowerBound(diff, Rational(1), 3);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::SAT);

  Rational d = s.computeDelta();
  Rational vx = s.getValue(x, d);
  Rational vy = s.getValue(y, d);
  ASSERT_GE(vx, Rational(0));
  ASSERT_GE(vy, Rational(0));
  ASSERT_LE(vx + vy, Rational(4));
  ASSERT_GE(vx - vy, Rational(1));
  ASSERT_EQ(s.getValue(sum, d), vx + vy);
  ASSERT_EQ(s.getValue(diff, d), vx - vy);
}

TEST_F(TestTheoryWhiteArithExactSimplex, sat_unbounded)
{
  // 3x - 2y >= 7 with x, y unbounded
  ExactSimplex s;
  ArithVar x = s.addVariable();
  ArithVar y = s.addVariable();
  ArithVar r = s.addRow({{x, Rational(3)}, {y, Rational(-2)}});
  s.setLowerBound(r, Rational(7), 0);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::SAT);
  Rational d = s.computeDelta();
  ASSERT_GE(Rational(3) * s.getValue(x, d) - Rational(2) * s.getValue(y, d),
            Rational(7));
}

TEST_F(TestTheoryWhiteArithExactSimplex, crossing_bounds)
{
  // x + y >= 5 and x + y <= 3 on the same row
  ExactSimplex s;
  ArithVar x = s.addVariable();
  ArithVar y = s.addVariable();
  ArithVar r = s.addRow({{x, Rational(1)}, {y, Rational(1)}});
  s.setLowerBound(r, Rational(5), 7);
  s.setUpperBound(r, Rational(3), 8);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::INFEASIBLE);
  ASSERT_EQ(explanation(s), (std::set<size_t>{7, 8}));
}

TEST_F(TestTheoryWhiteArithExactSimplex, infeasible_farkas)
{
  // x >= 0 (0), y >= 0 (1), x + y <= 1 (2), x - y >= 2 (3).
  // (2) - (3) gives 2y <= -1, which contradicts (1); (0) is not needed.
  ExactSimplex s;
  ArithVar x = s.addVariable();
  ArithVar y = s.addVariable();
  ArithVar sum = s.addRow({{x, Rational(1)}, {y, Rational(1)}});
  ArithVar diff = s.addRow({{x, Rational(1)}, {y, Rational(-1)}});
  s.setLowerBound(x, Rational(0), 0);
  s.setLowerBound(y, Rational(0), 1);
  s.setUpperBound(sum, Rational(1), 2);
  s.setLowerBound(diff, Rational(2), 3);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::INFEASIBLE);
  ASSERT_EQ(explanation(s), (std::set<size_t>{1, 2, 3}));
}

TEST_F(TestTheoryWhiteArithExactSimplex, strict_infeasible)
{
  // x - y < 0 and y - x < 0 are infeasible only because they are strict.
  ExactSimplex s;
  ArithVar x = s.addVariable();
  ArithVar y = s.addVariable();
  ArithVar xy = s.addRow({{x, Rational(1)}, {y, Rational(-1)}});
  ArithVar yx = s.addRow({{y, Rational(1)}, {x, Rational(-1)}});
  s.setUpperBound(xy, below(0), 0);
  s.setUpperBound(yx, below(0), 1);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::INFEASIBLE);
  ASSERT_EQ(explanation(s), (std::set<size_t>{0, 1}));

  // The non-strict versions are satisfiable, with x = y.
  s.setUpperBound(xy, Rational(0), 0);
  s.setUpperBound(yx, Rational(0), 1);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::SAT);
  Rational d = s.computeDelta();
  ASSERT_EQ(s.getValue(x, d), s.getValue(y, d));
}

TEST_F(TestTheoryWhiteArithExactSimplex, strict_sat_concrete_delta)
{
  // 0 < x < 1 and x + y > 2 with y < 1: needs strictly interior values.
  ExactSimplex s;
  ArithVar x = s.addVariable();
  ArithVar y = s.addVariable();
  ArithVar r = s.addRow({{x, Rational(1)}, {y, Rational(1)}});
  s.setLowerBound(x, above(0), 0);
  s.setUpperBound(x, below(1), 1);
  s.setUpperBound(y, below(1), 2);
  s.setLowerBound(r, above(1), 3);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::SAT);
  Rational d = s.computeDelta();
  ASSERT_GT(d, Rational(0));
  Rational vx = s.getValue(x, d);
  Rational vy = s.getValue(y, d);
  ASSERT_GT(vx, Rational(0));
  ASSERT_LT(vx, Rational(1));
  ASSERT_LT(vy, Rational(1));
  ASSERT_GT(vx + vy, Rational(1));
}

TEST_F(TestTheoryWhiteArithExactSimplex, pivot_limit)
{
  // Satisfying the row bound needs at least one pivot.
  ExactSimplex s;
  ArithVar x = s.addVariable();
  ArithVar r = s.addRow({{x, Rational(2)}});
  s.setLowerBound(r, Rational(4), 0);
  ASSERT_EQ(s.check(0), ExactSimplex::Status::UNKNOWN);
  ASSERT_EQ(s.check(10), ExactSimplex::Status::SAT);
}

TEST_F(TestTheoryWhiteArithExactSimplex, incremental_bounds)
{
  // x + y <= 10, x >= 0, y >= 0; then x >= 8, then y >= 3 (infeasible), then
  // the bound on y is removed again.
  ExactSimplex s;
  ArithVar x = s.addVariable();
  ArithVar y = s.addVariable();
  ArithVar r = s.addRow({{x, Rational(1)}, {y, Rational(1)}});
  s.setUpperBound(r, Rational(10), 0);
  s.setLowerBound(x, Rational(0), 1);
  s.setLowerBound(y, Rational(0), 2);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::SAT);
  s.setLowerBound(x, Rational(8), 1);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::SAT);
  s.setLowerBound(y, Rational(3), 2);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::INFEASIBLE);
  ASSERT_EQ(explanation(s), (std::set<size_t>{0, 1, 2}));
  s.removeLowerBound(y);
  ASSERT_EQ(s.check(100), ExactSimplex::Status::SAT);
  Rational d = s.computeDelta();
  ASSERT_GE(s.getValue(x, d), Rational(8));
  ASSERT_LE(s.getValue(r, d), Rational(10));
}

TEST_F(TestTheoryWhiteArithExactSimplex, random_systems)
{
  // On random systems, a SAT answer is checked against the original rows and
  // bounds, and an INFEASIBLE answer by re-solving only the explanation.
  std::mt19937 gen(42);
  std::uniform_int_distribution<int> coeff(-3, 3);
  std::uniform_int_distribution<int> bound(-6, 6);
  std::uniform_int_distribution<int> kind(0, 4);
  size_t numSat = 0, numInfeasible = 0;
  for (size_t round = 0; round < 300; ++round)
  {
    size_t n = 2 + round % 4, m = 1 + round % 5;
    // Each bound: (variable index, is lower, value).
    std::vector<std::tuple<size_t, bool, DeltaRational>> bounds;
    std::vector<Row> rows;
    for (size_t i = 0; i < m; ++i)
    {
      Row row;
      for (size_t j = 0; j < n; ++j)
      {
        int c = coeff(gen);
        if (c != 0)
        {
          row.emplace_back(j, Rational(c));
        }
      }
      if (row.empty())
      {
        row.emplace_back(0, Rational(1));
      }
      rows.push_back(row);
    }
    for (size_t v = 0; v < n + m; ++v)
    {
      int k = kind(gen);
      int b = bound(gen);
      // k == 4 gives strict bounds
      int strict = (k == 4) ? 1 : 0;
      if (k == 0 || k == 2 || k == 4)
      {
        bounds.emplace_back(v, true, DeltaRational(b, strict));
      }
      if (k == 1 || k == 2 || k == 4)
      {
        bounds.emplace_back(
            v, false, DeltaRational(b + (k == 2 ? 2 : 1), -strict));
      }
    }

    auto build = [&](ExactSimplex& s, const std::vector<size_t>& use) {
      for (size_t j = 0; j < n; ++j)
      {
        s.addVariable();
      }
      for (const Row& row : rows)
      {
        s.addRow(row);
      }
      for (size_t id : use)
      {
        const auto& [v, isLower, b] = bounds[id];
        if (isLower)
        {
          s.setLowerBound(v, b, id);
        }
        else
        {
          s.setUpperBound(v, b, id);
        }
      }
    };

    std::vector<size_t> all(bounds.size());
    for (size_t i = 0; i < all.size(); ++i)
    {
      all[i] = i;
    }
    ExactSimplex s;
    build(s, all);
    ExactSimplex::Status st = s.check(10000);
    ASSERT_NE(st, ExactSimplex::Status::UNKNOWN);
    if (st == ExactSimplex::Status::SAT)
    {
      ++numSat;
      Rational d = s.computeDelta();
      std::vector<Rational> val(n + m);
      for (size_t j = 0; j < n; ++j)
      {
        val[j] = s.getValue(j, d);
      }
      for (size_t i = 0; i < m; ++i)
      {
        Rational sum;
        for (const auto& [x, a] : rows[i])
        {
          sum += a * val[x];
        }
        val[n + i] = sum;
        ASSERT_EQ(s.getValue(n + i, d), sum);
      }
      // A bound c + k delta with k != 0 stands for the strict bound on c.
      for (const auto& [v, isLower, b] : bounds)
      {
        const Rational& c = b.getNoninfinitesimalPart();
        if (isLower)
        {
          ASSERT_TRUE(b.infinitesimalSgn() > 0 ? val[v] > c : val[v] >= c);
        }
        else
        {
          ASSERT_TRUE(b.infinitesimalSgn() < 0 ? val[v] < c : val[v] <= c);
        }
      }
    }
    else
    {
      ++numInfeasible;
      std::vector<size_t> expl = s.getExplanation();
      ASSERT_FALSE(expl.empty());
      std::sort(expl.begin(), expl.end());
      ASSERT_TRUE(std::adjacent_find(expl.begin(), expl.end()) == expl.end());
      ExactSimplex t;
      build(t, expl);
      ASSERT_EQ(t.check(10000), ExactSimplex::Status::INFEASIBLE);
    }
  }
  // the generator produces both kinds of systems
  ASSERT_GT(numSat, 0u);
  ASSERT_GT(numInfeasible, 0u);
}

}  // namespace test
}  // namespace cvc5::internal

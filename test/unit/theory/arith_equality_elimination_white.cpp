/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * White box testing of the equality elimination used by the unit cube test.
 */

#include <random>
#include <set>
#include <utility>
#include <vector>

#include "test.h"
#include "theory/arith/linear/cube/equality_elimination.h"
#include "theory/arith/linear/cube/unit_cube.h"
#include "util/integer.h"
#include "util/rational.h"

namespace cvc5::internal {

using namespace theory::arith;
using namespace theory::arith::linear;

namespace test {

class TestTheoryWhiteArithEqualityElimination : public TestInternal
{
 protected:
  /** Whether the values satisfy all bounds of p. */
  static bool satisfies(const CubeProblem& p, const std::vector<Rational>& vals)
  {
    for (size_t v = 0, n = p.numVariables(); v < n; ++v)
    {
      if ((p.d_lower[v] && DeltaRational(vals[v]) < *p.d_lower[v])
          || (p.d_upper[v] && DeltaRational(vals[v]) > *p.d_upper[v]))
      {
        return false;
      }
    }
    return true;
  }

  /** Whether the values satisfy the equalities (equal bounds) of p. */
  static bool satisfiesEqualities(const CubeProblem& p,
                                  const std::vector<Rational>& vals)
  {
    for (size_t v = 0, n = p.numVariables(); v < n; ++v)
    {
      if (p.d_lower[v] && p.d_upper[v] && *p.d_lower[v] == *p.d_upper[v]
          && DeltaRational(vals[v]) != *p.d_lower[v])
      {
        return false;
      }
    }
    return true;
  }

  /** All points of {-b, ..., b}^n. */
  static std::vector<std::vector<Integer>> box(size_t n, int b)
  {
    std::vector<std::vector<Integer>> points = {{}};
    for (size_t j = 0; j < n; ++j)
    {
      std::vector<std::vector<Integer>> next;
      for (const auto& pt : points)
      {
        for (int c = -b; c <= b; ++c)
        {
          next.push_back(pt);
          next.back().emplace_back(c);
        }
      }
      points = std::move(next);
    }
    return points;
  }

  static bool inBox(const std::vector<Integer>& x, int b)
  {
    for (const Integer& c : x)
    {
      if (c.abs() > Integer(b))
      {
        return false;
      }
    }
    return true;
  }
};

TEST_F(TestTheoryWhiteArithEqualityElimination, unit_coefficient)
{
  // x + y = 3, 2x - 3y >= 1: y is solved for, which leaves one variable.
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t s = p.addRow({{x, Rational(1)}, {y, Rational(1)}});
  size_t r = p.addRow({{x, Rational(2)}, {y, Rational(-3)}});
  p.d_lower[s] = Rational(3);
  p.d_upper[s] = Rational(3);
  p.d_lower[r] = Rational(1);
  EliminationResult res = eliminateEqualities(p, 1000);
  ASSERT_EQ(res.d_status, EliminationResult::Status::REDUCED);
  ASSERT_EQ(res.d_equalities, 1u);
  ASSERT_EQ(res.d_fresh, 0u);
  ASSERT_EQ(res.d_reduced.d_numInputs, 1u);
  ASSERT_EQ(res.d_map.size(), 2u);
  for (int t = -10; t <= 10; ++t)
  {
    std::vector<Integer> xy = applyMap(res.d_map, {Integer(t)});
    std::vector<Rational> vals = p.evaluate(xy);
    ASSERT_EQ(vals[s], Rational(3));
    // The bounds of p hold iff those of the reduced problem do.
    ASSERT_EQ(satisfies(p, vals),
              satisfies(res.d_reduced, res.d_reduced.evaluate({Integer(t)})));
  }
}

TEST_F(TestTheoryWhiteArithEqualityElimination, decomposition)
{
  // 3x + 5y = 7 has no coefficient of 1, so a fresh variable is needed. Its
  // integer solutions are x = 4 + 5t, y = -1 - 3t.
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r = p.addRow({{x, Rational(3)}, {y, Rational(5)}});
  p.d_lower[r] = Rational(7);
  p.d_upper[r] = Rational(7);
  EliminationResult res = eliminateEqualities(p, 1000);
  ASSERT_EQ(res.d_status, EliminationResult::Status::REDUCED);
  ASSERT_GE(res.d_fresh, 1u);
  ASSERT_EQ(res.d_reduced.d_numInputs, 1u);
  std::set<std::vector<Integer>> images;
  for (int t = -50; t <= 50; ++t)
  {
    std::vector<Integer> xy = applyMap(res.d_map, {Integer(t)});
    ASSERT_EQ(p.evaluate(xy)[r], Rational(7));
    images.insert(xy);
  }
  // The map is injective, and reaches every solution in a box.
  ASSERT_EQ(images.size(), 101u);
  for (int t = -5; t <= 5; ++t)
  {
    std::vector<Integer> sol = {Integer(4 + 5 * t), Integer(-1 - 3 * t)};
    ASSERT_TRUE(images.count(sol)) << "t = " << t;
  }
}

TEST_F(TestTheoryWhiteArithEqualityElimination, infeasible_gcd)
{
  // 2x + 4y = 3 has no integer solution.
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r = p.addRow({{x, Rational(2)}, {y, Rational(4)}});
  p.d_lower[r] = Rational(3);
  p.d_upper[r] = Rational(3);
  ASSERT_EQ(eliminateEqualities(p, 1000).d_status,
            EliminationResult::Status::INFEASIBLE);
}

TEST_F(TestTheoryWhiteArithEqualityElimination, infeasible_fractional_value)
{
  // x = 1/2 for an integer x.
  CubeProblem p;
  size_t x = p.addInput();
  p.d_lower[x] = Rational(1, 2);
  p.d_upper[x] = Rational(1, 2);
  ASSERT_EQ(eliminateEqualities(p, 1000).d_status,
            EliminationResult::Status::INFEASIBLE);
}

TEST_F(TestTheoryWhiteArithEqualityElimination, infeasible_constant_bound)
{
  // x = 2 and 2x <= 3
  CubeProblem p;
  size_t x = p.addInput();
  size_t r = p.addRow({{x, Rational(2)}});
  p.d_lower[x] = Rational(2);
  p.d_upper[x] = Rational(2);
  p.d_upper[r] = Rational(3);
  ASSERT_EQ(eliminateEqualities(p, 1000).d_status,
            EliminationResult::Status::INFEASIBLE);
}

TEST_F(TestTheoryWhiteArithEqualityElimination, rational_coefficients)
{
  // x/2 + y/3 = 1, i.e., 3x + 2y = 6
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r = p.addRow({{x, Rational(1, 2)}, {y, Rational(1, 3)}});
  p.d_lower[r] = Rational(1);
  p.d_upper[r] = Rational(1);
  EliminationResult res = eliminateEqualities(p, 1000);
  ASSERT_EQ(res.d_status, EliminationResult::Status::REDUCED);
  for (int t = -10; t <= 10; ++t)
  {
    ASSERT_EQ(p.evaluate(applyMap(res.d_map, {Integer(t)}))[r], Rational(1));
  }
}

TEST_F(TestTheoryWhiteArithEqualityElimination, step_limit)
{
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r = p.addRow({{x, Rational(3)}, {y, Rational(5)}});
  p.d_lower[r] = Rational(7);
  p.d_upper[r] = Rational(7);
  ASSERT_EQ(eliminateEqualities(p, 0).d_status,
            EliminationResult::Status::UNKNOWN);
}

TEST_F(TestTheoryWhiteArithEqualityElimination, compose)
{
  // x = 1 + 2y, y = 3 - z, so x = 7 - 2z
  std::vector<AffineExpr> outer = {{Integer(1), {{0, Integer(2)}}}};
  std::vector<AffineExpr> inner = {{Integer(3), {{0, Integer(-1)}}}};
  std::vector<AffineExpr> c = composeMaps(outer, inner);
  for (int z = -5; z <= 5; ++z)
  {
    ASSERT_EQ(applyMap(c, {Integer(z)}),
              applyMap(outer, applyMap(inner, {Integer(z)})));
  }
}

TEST_F(TestTheoryWhiteArithEqualityElimination, then_unit_cube)
{
  // x + y = 3, 2x - 3y >= 1, x - 4y <= 20: no cube fits, but one fits after
  // the elimination, and its point satisfies the original problem.
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t s = p.addRow({{x, Rational(1)}, {y, Rational(1)}});
  size_t r1 = p.addRow({{x, Rational(2)}, {y, Rational(-3)}});
  size_t r2 = p.addRow({{x, Rational(1)}, {y, Rational(-4)}});
  p.d_lower[s] = Rational(3);
  p.d_upper[s] = Rational(3);
  p.d_lower[r1] = Rational(1);
  p.d_upper[r2] = Rational(20);
  ASSERT_EQ(runUnitCubeTest(p, 1000).d_status, CubeResult::Status::NO_ROOM);
  EliminationResult res = eliminateEqualities(p, 1000);
  ASSERT_EQ(res.d_status, EliminationResult::Status::REDUCED);
  CubeResult cube = runUnitCubeTest(res.d_reduced, 1000);
  ASSERT_EQ(cube.d_status, CubeResult::Status::SUCCESS);
  ASSERT_TRUE(satisfies(p, p.evaluate(applyMap(res.d_map, cube.d_point))));
}

TEST_F(TestTheoryWhiteArithEqualityElimination, random_systems)
{
  // Random equalities over 2 or 3 variables. The integer solutions in a box
  // are computed by brute force and must be exactly the images of the map
  // that lie in the box, each reached once. Random inequalities check that
  // the bounds of the reduced problem hold iff those of p do.
  std::mt19937 gen(17);
  std::uniform_int_distribution<int> coeff(-6, 6);
  std::uniform_int_distribution<int> constant(-6, 6);
  std::uniform_int_distribution<int> bound(-10, 10);
  const int b = 5, k = 40;
  size_t numReduced = 0, numInfeasible = 0, numFresh = 0;
  for (size_t round = 0; round < 200; ++round)
  {
    size_t n = 2 + round % 2;
    size_t m = 1 + (round / 2) % (n - 1);
    CubeProblem p;
    for (size_t j = 0; j < n; ++j)
    {
      p.addInput();
    }
    std::vector<size_t> eqs;
    for (size_t i = 0; i < m; ++i)
    {
      std::vector<std::pair<size_t, Rational>> row;
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
        row.emplace_back(0, Rational(2));
      }
      size_t v = p.addRow(row);
      p.d_lower[v] = Rational(constant(gen));
      p.d_upper[v] = p.d_lower[v];
      eqs.push_back(v);
    }
    // an inequality over all variables
    std::vector<std::pair<size_t, Rational>> row;
    for (size_t j = 0; j < n; ++j)
    {
      row.emplace_back(j, Rational(coeff(gen)));
    }
    size_t ineq = p.addRow(row);
    p.d_upper[ineq] = Rational(bound(gen));

    std::set<std::vector<Integer>> solutions;
    bool anyFeasible = false;
    for (const auto& pt : box(n, b))
    {
      std::vector<Rational> vals = p.evaluate(pt);
      if (satisfiesEqualities(p, vals))
      {
        solutions.insert(pt);
        anyFeasible |= satisfies(p, vals);
      }
    }

    EliminationResult res = eliminateEqualities(p, 1000);
    ASSERT_NE(res.d_status, EliminationResult::Status::UNKNOWN);
    if (res.d_status == EliminationResult::Status::INFEASIBLE)
    {
      ++numInfeasible;
      // p must have no integer solution.
      ASSERT_FALSE(anyFeasible) << "round " << round;
      continue;
    }
    ++numReduced;
    numFresh += res.d_fresh;
    size_t numY = res.d_reduced.d_numInputs;
    ASSERT_LT(numY, n) << "round " << round;
    std::set<std::vector<Integer>> images;
    size_t numImagesInBox = 0;
    for (const auto& y : box(numY, k))
    {
      std::vector<Integer> x = applyMap(res.d_map, y);
      std::vector<Rational> vals = p.evaluate(x);
      ASSERT_TRUE(satisfiesEqualities(p, vals)) << "round " << round;
      ASSERT_EQ(satisfies(p, vals),
                satisfies(res.d_reduced, res.d_reduced.evaluate(y)))
          << "round " << round;
      if (inBox(x, b))
      {
        ++numImagesInBox;
        images.insert(x);
      }
    }
    ASSERT_EQ(images, solutions) << "round " << round;
    ASSERT_EQ(numImagesInBox, solutions.size()) << "round " << round;
  }
  // Both outcomes and the decomposition are exercised.
  ASSERT_GT(numReduced, 50u);
  ASSERT_GT(numInfeasible, 5u);
  ASSERT_GT(numFresh, 20u);
}

}  // namespace test
}  // namespace cvc5::internal

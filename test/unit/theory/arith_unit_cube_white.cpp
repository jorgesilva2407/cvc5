/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * White box testing of the unit cube test and the implied equality
 * detection.
 */

#include <map>
#include <random>
#include <utility>
#include <vector>

#include "test.h"
#include "theory/arith/linear/cube/exact_simplex.h"
#include "theory/arith/linear/cube/unit_cube.h"
#include "util/integer.h"
#include "util/rational.h"

namespace cvc5::internal {

using namespace theory::arith;
using namespace theory::arith::linear;

namespace test {

class TestTheoryWhiteArithUnitCube : public TestInternal
{
 protected:
  /** Whether the values satisfy all bounds of p. */
  static bool satisfies(const CubeProblem& p, const std::vector<Rational>& vals)
  {
    for (size_t v = 0, n = p.numVariables(); v < n; ++v)
    {
      if ((p.d_lower[v] && vals[v] < p.d_lower[v]->getNoninfinitesimalPart())
          || (p.d_upper[v]
              && vals[v] > p.d_upper[v]->getNoninfinitesimalPart()))
      {
        return false;
      }
    }
    return true;
  }

  /** Whether the bounds of p (possibly strict) are satisfiable. */
  static bool feasible(const CubeProblem& p)
  {
    ExactSimplex s;
    std::vector<ArithVar> vars;
    for (size_t j = 0; j < p.d_numInputs; ++j)
    {
      vars.push_back(s.addVariable());
    }
    for (const auto& row : p.d_rows)
    {
      std::vector<std::pair<ArithVar, Rational>> coeffs;
      for (const auto& [x, a] : row)
      {
        coeffs.emplace_back(vars[x], a);
      }
      vars.push_back(s.addRow(coeffs));
    }
    for (size_t v = 0, n = p.numVariables(); v < n; ++v)
    {
      if (p.d_lower[v])
      {
        s.setLowerBound(vars[v], *p.d_lower[v], 2 * v);
      }
      if (p.d_upper[v])
      {
        s.setUpperBound(vars[v], *p.d_upper[v], 2 * v + 1);
      }
    }
    ExactSimplex::Status st = s.check(100000);
    EXPECT_NE(st, ExactSimplex::Status::UNKNOWN);
    return st == ExactSimplex::Status::SAT;
  }

  /** A problem with random rows over 2 to 4 inputs. */
  static CubeProblem randomRows(std::mt19937& gen)
  {
    std::uniform_int_distribution<int> coeff(-4, 4);
    std::uniform_int_distribution<size_t> count(2, 4);
    CubeProblem p;
    size_t n = count(gen);
    for (size_t j = 0; j < n; ++j)
    {
      p.addInput();
    }
    for (size_t i = 0, m = count(gen) - 1; i < m; ++i)
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
        row.emplace_back(0, Rational(1));
      }
      p.addRow(row);
    }
    return p;
  }

  /** The equalities found, as a map from variable to value. */
  static std::map<size_t, Rational> equalities(const ImpliedEqResult& r)
  {
    std::map<size_t, Rational> m;
    for (const ImpliedEquality& eq : r.d_equalities)
    {
      m[eq.d_var] = eq.d_value;
    }
    return m;
  }
};

TEST_F(TestTheoryWhiteArithUnitCube, shift)
{
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r = p.addRow({{x, Rational(3)}, {y, Rational(-7)}});
  ASSERT_EQ(p.shift(x), Rational(1, 2));
  ASSERT_EQ(p.shift(r), Rational(5));
}

TEST_F(TestTheoryWhiteArithUnitCube, unbounded_success)
{
  // 3x - 7y >= 1, 2x + 5y >= 3: infinite lattice width, a cube fits.
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r1 = p.addRow({{x, Rational(3)}, {y, Rational(-7)}});
  size_t r2 = p.addRow({{x, Rational(2)}, {y, Rational(5)}});
  p.d_lower[r1] = Rational(1);
  p.d_lower[r2] = Rational(3);
  CubeResult res = runUnitCubeTest(p, 1000);
  ASSERT_EQ(res.d_status, CubeResult::Status::SUCCESS);
  ASSERT_EQ(res.d_point.size(), 2u);
  ASSERT_TRUE(satisfies(p, p.evaluate(res.d_point)));
}

TEST_F(TestTheoryWhiteArithUnitCube, paper_example_no_cube)
{
  // Bromberger and Weidenbach (IJCAR 2016): the only integer solution is
  // (1, 3) and no unit cube fits.
  CubeProblem p;
  size_t x1 = p.addInput();
  size_t x2 = p.addInput();
  size_t r1 = p.addRow({{x1, Rational(3)}, {x2, Rational(-1)}});
  size_t r2 = p.addRow({{x1, Rational(-2)}, {x2, Rational(-1)}});
  size_t r3 = p.addRow({{x1, Rational(-2)}, {x2, Rational(1)}});
  p.d_upper[r1] = Rational(0);
  p.d_upper[r2] = Rational(-2);
  p.d_upper[r3] = Rational(1);
  CubeResult res = runUnitCubeTest(p, 1000);
  ASSERT_EQ(res.d_status, CubeResult::Status::SHIFTED_INFEASIBLE);
  // There are no implied equalities.
  ImpliedEqResult eq = detectImpliedEqualities(p, 1000);
  ASSERT_TRUE(eq.d_complete);
  ASSERT_TRUE(eq.d_equalities.empty());
  ASSERT_EQ(eq.d_rounds, 1u);
}

TEST_F(TestTheoryWhiteArithUnitCube, explicit_equality_no_room)
{
  // x + y = 3, 2x - 3y >= 1
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r1 = p.addRow({{x, Rational(1)}, {y, Rational(1)}});
  size_t r2 = p.addRow({{x, Rational(2)}, {y, Rational(-3)}});
  p.d_lower[r1] = Rational(3);
  p.d_upper[r1] = Rational(3);
  p.d_lower[r2] = Rational(1);
  ASSERT_EQ(runUnitCubeTest(p, 1000).d_status, CubeResult::Status::NO_ROOM);
  // The explicit equality is not reported as implied, and there are no
  // others.
  ImpliedEqResult eq = detectImpliedEqualities(p, 1000);
  ASSERT_TRUE(eq.d_complete);
  ASSERT_TRUE(eq.d_equalities.empty());
}

TEST_F(TestTheoryWhiteArithUnitCube, implied_cycle)
{
  // x - y <= 0, y - z <= 0, x - z >= 0 imply x = y = z; 2x + 3y >= 1 does
  // not hold with equality.
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t z = p.addInput();
  size_t xy = p.addRow({{x, Rational(1)}, {y, Rational(-1)}});
  size_t yz = p.addRow({{y, Rational(1)}, {z, Rational(-1)}});
  size_t xz = p.addRow({{x, Rational(1)}, {z, Rational(-1)}});
  size_t r = p.addRow({{x, Rational(2)}, {y, Rational(3)}});
  p.d_upper[xy] = Rational(0);
  p.d_upper[yz] = Rational(0);
  p.d_lower[xz] = Rational(0);
  p.d_lower[r] = Rational(1);
  ASSERT_NE(runUnitCubeTest(p, 1000).d_status, CubeResult::Status::SUCCESS);
  ImpliedEqResult eq = detectImpliedEqualities(p, 1000);
  ASSERT_TRUE(eq.d_complete);
  std::map<size_t, Rational> expected = {
      {xy, Rational(0)}, {yz, Rational(0)}, {xz, Rational(0)}};
  ASSERT_EQ(equalities(eq), expected);
  ASSERT_EQ(eq.d_equalities.size(), 3u);
}

TEST_F(TestTheoryWhiteArithUnitCube, implied_variable_bounds)
{
  // x + y <= 2, x >= 1, y >= 1 imply x = 1, y = 1 and x + y = 2;
  // 2z - x >= 2 does not hold with equality.
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t z = p.addInput();
  size_t s = p.addRow({{x, Rational(1)}, {y, Rational(1)}});
  size_t r = p.addRow({{z, Rational(2)}, {x, Rational(-1)}});
  p.d_upper[s] = Rational(2);
  p.d_lower[x] = Rational(1);
  p.d_lower[y] = Rational(1);
  p.d_lower[r] = Rational(2);
  ASSERT_EQ(runUnitCubeTest(p, 1000).d_status,
            CubeResult::Status::SHIFTED_INFEASIBLE);
  ImpliedEqResult eq = detectImpliedEqualities(p, 1000);
  ASSERT_TRUE(eq.d_complete);
  std::map<size_t, Rational> expected = {
      {x, Rational(1)}, {y, Rational(1)}, {s, Rational(2)}};
  ASSERT_EQ(equalities(eq), expected);
}

TEST_F(TestTheoryWhiteArithUnitCube, no_implied_equalities)
{
  // 3x - 7y >= 1, 2x + 5y >= 3 has an interior.
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r1 = p.addRow({{x, Rational(3)}, {y, Rational(-7)}});
  size_t r2 = p.addRow({{x, Rational(2)}, {y, Rational(5)}});
  p.d_lower[r1] = Rational(1);
  p.d_lower[r2] = Rational(3);
  ImpliedEqResult eq = detectImpliedEqualities(p, 1000);
  ASSERT_TRUE(eq.d_complete);
  ASSERT_TRUE(eq.d_equalities.empty());
  ASSERT_EQ(eq.d_rounds, 1u);
}

TEST_F(TestTheoryWhiteArithUnitCube, pivot_limit)
{
  CubeProblem p;
  size_t x = p.addInput();
  size_t y = p.addInput();
  size_t r1 = p.addRow({{x, Rational(3)}, {y, Rational(-7)}});
  size_t r2 = p.addRow({{x, Rational(2)}, {y, Rational(5)}});
  p.d_lower[r1] = Rational(1);
  p.d_lower[r2] = Rational(3);
  ASSERT_EQ(runUnitCubeTest(p, 0).d_status, CubeResult::Status::UNKNOWN);
  ASSERT_FALSE(detectImpliedEqualities(p, 0).d_complete);
}

TEST_F(TestTheoryWhiteArithUnitCube, random_cube_fits)
{
  // Every bound is at least as far from a random integer point z0 as the
  // shift, so z0 is a solution of the shifted system and the test must
  // succeed.
  std::mt19937 gen(7);
  std::uniform_int_distribution<int> point(-5, 5);
  std::uniform_int_distribution<int> extra(0, 2);
  std::uniform_int_distribution<int> kind(0, 3);
  for (size_t round = 0; round < 300; ++round)
  {
    CubeProblem p = randomRows(gen);
    std::vector<Integer> z0;
    for (size_t j = 0; j < p.d_numInputs; ++j)
    {
      z0.emplace_back(point(gen));
    }
    std::vector<Rational> val = p.evaluate(z0);
    for (size_t v = 0, n = p.numVariables(); v < n; ++v)
    {
      int k = kind(gen);
      Rational h = p.shift(v);
      if (k == 1 || k == 3)
      {
        p.d_lower[v] = Rational((val[v] - h).floor() - Integer(extra(gen)));
      }
      if (k == 2 || k == 3)
      {
        p.d_upper[v] = Rational((val[v] + h).ceiling() + Integer(extra(gen)));
      }
    }
    CubeResult res = runUnitCubeTest(p, 10000);
    ASSERT_EQ(res.d_status, CubeResult::Status::SUCCESS) << "round " << round;
    ASSERT_TRUE(satisfies(p, p.evaluate(res.d_point))) << "round " << round;
  }
}

TEST_F(TestTheoryWhiteArithUnitCube, random_cube_sound)
{
  // On random problems, a point is only returned if it satisfies every
  // bound, and NO_ROOM only if some shifted bounds really cross.
  std::mt19937 gen(11);
  std::uniform_int_distribution<int> bound(-8, 8);
  std::uniform_int_distribution<int> width(0, 12);
  std::uniform_int_distribution<int> kind(0, 3);
  size_t numSuccess = 0;
  for (size_t round = 0; round < 500; ++round)
  {
    CubeProblem p = randomRows(gen);
    for (size_t v = 0, n = p.numVariables(); v < n; ++v)
    {
      int k = kind(gen);
      int b = bound(gen);
      if (k == 1 || k == 3)
      {
        p.d_lower[v] = Rational(b);
      }
      if (k == 2 || k == 3)
      {
        p.d_upper[v] = Rational(b + (k == 3 ? width(gen) : 0));
      }
    }
    CubeResult res = runUnitCubeTest(p, 10000);
    ASSERT_NE(res.d_status, CubeResult::Status::UNKNOWN);
    ASSERT_NE(res.d_status, CubeResult::Status::ROUND_FAILED);
    if (res.d_status == CubeResult::Status::SUCCESS)
    {
      ++numSuccess;
      ASSERT_TRUE(satisfies(p, p.evaluate(res.d_point))) << "round " << round;
    }
    if (res.d_status == CubeResult::Status::NO_ROOM)
    {
      bool crossing = false;
      for (size_t v = 0, n = p.numVariables(); v < n; ++v)
      {
        crossing |= p.d_lower[v] && p.d_upper[v]
                    && p.d_lower[v]->getNoninfinitesimalPart() + p.shift(v)
                           > p.d_upper[v]->getNoninfinitesimalPart()
                                 - p.shift(v);
      }
      ASSERT_TRUE(crossing) << "round " << round;
    }
  }
  // The test is exercised.
  ASSERT_GT(numSuccess, 20u);
}

TEST_F(TestTheoryWhiteArithUnitCube, random_implied_equalities)
{
  // Random problems satisfied by a point x0, where some bounds are tight at
  // x0. The reported equalities are checked one by one (v = c must hold at
  // x0, and neither v < c nor v > c is satisfiable), and every other bound
  // must be satisfiable strictly on its own.
  std::mt19937 gen(13);
  std::uniform_int_distribution<int> point(-6, 6);
  std::uniform_int_distribution<int> slack(1, 3);
  std::uniform_int_distribution<int> kind(0, 6);
  size_t numFound = 0;
  for (size_t round = 0; round < 300; ++round)
  {
    CubeProblem p = randomRows(gen);
    std::vector<Rational> x0;
    for (size_t j = 0; j < p.d_numInputs; ++j)
    {
      x0.emplace_back(point(gen), 2);
    }
    std::vector<Rational> val = x0;
    for (const auto& row : p.d_rows)
    {
      Rational sum;
      for (const auto& [x, a] : row)
      {
        sum += a * x0[x];
      }
      val.push_back(sum);
    }
    std::vector<bool> explicitEq(p.numVariables(), false);
    for (size_t v = 0, n = p.numVariables(); v < n; ++v)
    {
      switch (kind(gen))
      {
        case 1: p.d_lower[v] = val[v]; break;
        case 2: p.d_upper[v] = val[v]; break;
        case 3: p.d_lower[v] = val[v] - Rational(slack(gen)); break;
        case 4: p.d_upper[v] = val[v] + Rational(slack(gen)); break;
        case 5:
          p.d_lower[v] = val[v] - Rational(slack(gen));
          p.d_upper[v] = val[v] + Rational(slack(gen));
          break;
        case 6:
          p.d_lower[v] = val[v];
          p.d_upper[v] = val[v];
          explicitEq[v] = true;
          break;
        default: break;
      }
    }

    ImpliedEqResult eq = detectImpliedEqualities(p, 100000);
    ASSERT_TRUE(eq.d_complete) << "round " << round;
    std::map<size_t, Rational> found = equalities(eq);
    ASSERT_EQ(found.size(), eq.d_equalities.size()) << "round " << round;
    numFound += found.size();
    for (const auto& [v, c] : found)
    {
      ASSERT_FALSE(explicitEq[v]) << "round " << round;
      ASSERT_EQ(c, val[v]) << "round " << round;
      CubeProblem above = p;
      above.d_lower[v] = DeltaRational(c, 1);
      ASSERT_FALSE(feasible(above)) << "round " << round << " var " << v;
      CubeProblem below = p;
      below.d_upper[v] = DeltaRational(c, -1);
      ASSERT_FALSE(feasible(below)) << "round " << round << " var " << v;
    }
    for (size_t v = 0, n = p.numVariables(); v < n; ++v)
    {
      if (explicitEq[v] || found.count(v))
      {
        continue;
      }
      if (p.d_lower[v])
      {
        CubeProblem q = p;
        q.d_lower[v] = DeltaRational(p.d_lower[v]->getNoninfinitesimalPart(), 1);
        ASSERT_TRUE(feasible(q)) << "round " << round << " var " << v;
      }
      if (p.d_upper[v])
      {
        CubeProblem q = p;
        q.d_upper[v] =
            DeltaRational(p.d_upper[v]->getNoninfinitesimalPart(), -1);
        ASSERT_TRUE(feasible(q)) << "round " << round << " var " << v;
      }
    }
  }
  // Implied equalities do occur.
  ASSERT_GT(numFound, 20u);
}

}  // namespace test
}  // namespace cvc5::internal

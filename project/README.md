# Unit cube test in cvc5: status, design and plan

DCC071 course project, Lara Amélia Maia de Freitas and Jorge Augusto de Lima e
Silva. The proposal is [`_SMT__Proposal-1-4.pdf`](_SMT__Proposal-1-4.pdf). The
paper is M. Bromberger and C. Weidenbach, *Fast cube tests for LIA constraint
solving*, IJCAR 2016 (here "the paper" or "B&W").

This document describes the state of the work as of 2026-11-01: what is done,
the decisions we agreed on, how the implementation works, the results so far,
what comes next, and how everything is tested.

Contents:

1. [Summary](#1-summary)
2. [What we agreed on](#2-what-we-agreed-on)
3. [How it works](#3-how-it-works)
4. [Code map](#4-code-map)
5. [What has been done and checked](#5-what-has-been-done-and-checked)
6. [Results so far](#6-results-so-far)
7. [Known limitations and open questions](#7-known-limitations-and-open-questions)
8. [Plan for what comes next](#8-plan-for-what-comes-next)
9. [Testing plan](#9-testing-plan)
10. [How to build and run](#10-how-to-build-and-run)

---

## 1. Summary

All three components of the proposal are implemented in cvc5 1.4.1, each
behind its own option and off by default:

| component | option | what it does |
|---|---|---|
| C1 unit cube test | `--arith-cube-test` | shift the bounds inwards, solve the shifted system over the rationals, round its solution, and use it as the model if it satisfies every constraint |
| C2 implied equalities | `--arith-cube-implied-eq` | when the shifted system is infeasible, find the equalities implied by the constraints (B&W Lemma 6) |
| C3 equality elimination | `--arith-cube-eq-elim` | remove the equalities (explicit, and those found by C2) with a Diophantine solver, run C1 on the reduced problem, map the point back |
| (limit) | `--arith-cube-pivot-limit=N` (expert, default 10000) | pivots per simplex call of C1/C2, and steps of C3; the component gives up when reached |

Status:

- Builds: debug (assertions, unit tests) and optimized, both compile.
- 30 unit tests and 16 regressions, all passing.
- A mutation check (6 deliberate bugs) showed the tests catch each of them
  (done before C3 existed; C3 still needs its own, see §8).
- A sweep over cvc5's arithmetic regressions in the debug build with
  `--check-models --check-proofs` and all options on: same answers as plain
  cvc5, all checks pass.
- A first benchmark run (150 SMT-LIB files, 60 s): 0 wrong answers, all 351 sat
  models of our configurations verified, 132 → 140 files solved, and the time
  on the files everyone solves drops from 345 s to under 11 s.
- The work is on the branch `cube-test-v2`, based on the `cvc5-1.4.1` tag,
  in three commits: the standalone `cube/` library with its unit tests, its
  integration into cvc5 with the regressions, and this `project/` folder.

## 2. What we agreed on

These are the decisions taken so far, with the reason for each.

### Scope and safety rules (from the proposal)

- **Base:** cvc5 1.4.1, forked at its tag (the repository's `main` was already
  past 1.4.2).
- **Pure integer problems only.** The shifts are only correct if every
  variable is rounded, so the test checks at run time that all variables are
  integers and is skipped otherwise (counted as `notApplicable`).
- **Everything behind new options, off by default.** With the options off, the
  new code is never reached and cvc5 behaves exactly as 1.4.1.
- **Statistics** for every component: calls, successes, failures by reason,
  time.
- **Never produce `unsat` or a theory conflict from shifted or strict
  bounds.** Those bounds are not facts of the problem, so a conflict found
  with them is not a valid conflict. All three components work on a copy of
  the problem; anything they derive stays in that copy, and when they fail
  they simply give up and cvc5 continues as usual.
- **Every rounded point is checked against all original constraints,
  including disequalities**, before it is used.
- **Testing** with a debug build (assertions on) and `--check-models`.

### Design choices

- **(a) Where the shifted system is solved: in a separate simplex instance.**
  The proposal left open solving it in place (inside cvc5's own tableau,
  under a scoped push, as Jorge's first version did) or in a separate exact
  simplex. We chose the separate instance (`ExactSimplex`):
  - it cannot disturb cvc5's state, so the "no conflicts from shifted bounds"
    rule holds by construction rather than by careful cleanup;
  - C3 changes the variables of the problem, which the in-place approach
    cannot do anyway;
  - it is easy to test in isolation (unit tests run without a solver).

  Jorge agreed to replacing the in-place version. His commit was taken as the
  starting point (options, statistics, call site) and rebuilt on 1.4.1.
- **(b) How a successful point becomes the model: installed directly.** The
  point is written into cvc5's assignment through its existing
  `AttemptSolutionSDP` (the same mechanism cvc5 uses for approximate
  solutions), keeping the current basis. Every value is within its bounds, so
  this only moves nonbasic variables and cannot fail. The alternative from
  the proposal (emitting brab-style lemmas with the rounded value as the
  preferred phase) is kept as a possible variant for RQ3.
- **Proofs:** the test is allowed when proofs are enabled. It only changes
  the assignment and never derives a lemma or a conflict, so there is nothing
  to prove. (Jorge's version skipped the test with proofs on; that guard was
  removed.)
- **Follow the paper exactly.** The shift is ½‖aᵢ‖₁ for every row and ½ for
  every variable, rounding is ⌊z + ½⌋, with no variants of our own (for
  example, no special case for rows whose variables are all integers). Any
  deviation would have to be argued separately, and is not part of the
  project.
- **Pipeline order when there are equalities.** An equality leaves no room
  for a unit cube, so:
  1. first check whether the problem has an equality (a variable whose lower
     and upper bounds are equal);
  2. if it has one and C3 is off, skip the test (counted as
     `skippedEquality`) and continue with cvc5's normal pipeline;
  3. if C3 is on, eliminate the equalities and run the test on the reduced
     problem.

  C3 is an option of its own, so its effect can be measured by running with
  and without it.
- **C3 is a reimplementation of Griggio's method, decoupled from the
  solver.** cvc5 already has a Diophantine solver (`DioSolver`, after
  Griggio, JSAT 2012), but it only exposes substitutions without fresh
  variables, its full solved form is private, and it is tied to the SAT
  context and to conflict generation. C3 reimplements the same algorithm,
  following `DioSolver`'s structure and method names, but on a copy of the
  problem, with no connection to the SAT context, conflicts, lemmas or
  proofs, so it can run separately and independently. (The proposal's
  fallback, an own Hermite normal form elimination, was not needed.)

### How we work

- Changes are not committed until both of us have reviewed them; each of us
  should be able to explain every part (proposal §4).

## 3. How it works

### Where the test runs

In `TheoryArithPrivate::postCheck`, at full effort, when the real relaxation
is satisfiable but its model is not integral. It runs after cvc5's
Diophantine conflict check and before cvc5's own Diophantine cuts and
branching (branch-and-bound / brab), as in the proposal. If the test
succeeds, the model is integral and nothing is cut or branched on; if it
fails, cvc5 continues exactly as without it. The test runs at every such
check, so with Boolean structure it can run many times in one problem.

### The problem the test sees

`cubeSnapshot` copies cvc5's current linear problem into a `CubeProblem`:

- the **input variables**: the problem's own (non-auxiliary) variables;
- the **rows**: cvc5 introduces an auxiliary variable `s = a·x` for each
  linear term with a bound (e.g. `2x − 3y ≤ 4` becomes `s = 2x − 3y`,
  `s ≤ 4`). Each one is a row `a` over the input variables;
- the **bounds** currently asserted on each input and row variable.

In integer problems cvc5 turns strict bounds into non-strict ones (`x < 5`
becomes `x ≤ 4`), so all bounds in the snapshot are non-strict (checked by an
assertion). Disequalities are not part of the snapshot; they are only checked
on the rounded point.

### C1: the unit cube test (`runUnitCubeTest`)

The paper: the polyhedron `Ax ≤ b` contains an axis-parallel cube of edge 1,
and hence an integer point, iff the shifted system `Az ≤ b − ½‖aᵢ‖₁` has a
rational solution `z`; rounding `z` gives an integer point of `Ax ≤ b`.

1. Shift every bound inwards: by ½ for an input variable, by ½‖a‖₁ (half the
   sum of the absolute values of the row's coefficients) for a row. Lower
   bounds go up, upper bounds go down.
2. If some variable's shifted bounds cross, there is no room for a cube:
   `NO_ROOM`. This always happens with an equality (both bounds equal), and
   also for narrow ranges such as `1 ≤ 2x − 3y ≤ 4`, whose width 3 is less
   than the shift 2·½(2 + 3) = 5.
3. Otherwise solve the shifted system with `ExactSimplex`. If it is
   infeasible: `SHIFTED_INFEASIBLE` (no unit cube fits anywhere). If the
   pivot limit is reached: `UNKNOWN`.
4. Round every input to the nearest integer, ⌊z + ½⌋.
5. Compute every row's value at the rounded point and check it against the
   original bounds. By the paper's theorem this cannot fail; it is checked
   anyway (`ROUND_FAILED`, an assertion in debug builds).

Back in `TheoryArithPrivate`:

6. `cubeSatisfiesAll` checks the point against every bound of cvc5's problem
   and against every asserted disequality. A disequality can fail
   (`diseqFailed`); the test then gives up.
7. `cubeInstall` writes the point into cvc5's assignment (decision (b)).
   cvc5 then sees an integral model and answers `sat` as usual (and
   `--check-models` verifies it against the input formula).

### `ExactSimplex`: the separate simplex

A small standalone simplex over exact rationals, reusing cvc5's `Tableau`
class. It decides whether a set of bounds on variables and rows is feasible.

- Bland's rule for pivoting (simple, and it terminates), plus a pivot limit.
- Strict bounds (needed by C2) are written with cvc5's delta-rationals:
  `x < b` is `x ≤ b − δ` for a symbolic infinitesimal `δ`.
- Every bound carries an id. When the system is infeasible, it returns the
  explanation: the ids of the bounds in the conflicting row (those with a
  nonzero Farkas multiplier), whose conjunction is infeasible.
- It has no connection to the SAT solver or to cvc5's tableau; nothing it
  finds leaves the instance.

### C2: implied equalities (`detectImpliedEqualities`)

The paper's Lemma 6: if the problem is feasible but the strict system
(`<` instead of `≤`) is infeasible, every strict bound in the conflict
explanation holds with equality in every solution.

1. Make every bound strict (except the bounds of variables that are already
   fixed by an equality).
2. Solve. If feasible, there are no more implied equalities: done.
3. If infeasible, every strict bound in the explanation is an implied
   equality: fix that variable to that bound, and go back to 2.

Each round fixes at least one variable, so this ends. It is called only when
the shifted system was infeasible or had no room. Without C3, the equalities
found are only counted (they explain why the test failed, but are not used).
They are never turned into lemmas or conflicts.

### C3: equality elimination (`eliminateEqualities`)

Griggio's method, as in cvc5's `DioSolver`, on a copy of the problem:

1. Every variable with equal bounds gives an equation `a·x = c` (scaled to
   integer coefficients). These go into a queue.
2. Repeatedly take the equation with the coefficient of smallest absolute
   value:
   - if it is ±1, solve the equation for that variable and substitute it
     into all other equations (`solveIndex`);
   - otherwise, first try to combine equations into one with a coefficient of
     ±1 (`impliedGcdOfOne`);
   - otherwise, decompose the equation with a fresh variable (`decomposeIndex`):
     with the smallest coefficient `a` on `x`, divide every other
     coefficient and the constant by `a` with remainder (`aⱼ = a·qⱼ + rⱼ`,
     `c = a·q_c + r_c`, floor division) and substitute
     `x = σ − Σ qⱼ·xⱼ − q_c` with a new variable `σ`. The equation becomes
     `a·σ + Σ rⱼ·xⱼ + r_c = 0`, whose other coefficients are smaller than
     `a`; repeat until a coefficient becomes ±1.
   - after each substitution, divide equations by the gcd of their
     coefficients; if the gcd does not divide the constant, the equations
     have no integer solution (`INFEASIBLE`).
3. The result is the solved form `x = c + T·y`: every original input as an
   integer affine expression (`AffineExpr`) over new variables `y`.
4. Substitute it into every bound of the problem. This gives a
   `CubeProblem` over `y` with no equalities left. If an expression becomes a
   constant outside its bounds, the problem is `INFEASIBLE`.

Its integer solutions `y` correspond exactly to the integer solutions
`x = c + T·y` of the original problem. The unit cube test runs on the reduced
problem, and its point `y` is mapped back to `x` and checked against all the
original constraints as in C1.

`INFEASIBLE` and `UNKNOWN` (step limit) only make the test give up. Even
though `INFEASIBLE` means the current bounds have no integer solution, it is
never reported as a conflict (safety rule); cvc5 finds that out by itself.

### The whole pipeline (`unitCubeTest`)

```
if C3 is off and the problem has an equality:
    skip (skippedEquality), continue with cvc5
p := snapshot; q := p; map := identity
loop:
    if C3 is on and q has an equality:
        eliminate them (q, map := reduced problem, composed map)
        if infeasible or step limit: give up
    run C1 on q
    if success: leave the loop
    if no room or shifted system infeasible:
        if C2 is off: give up
        find the implied equalities of q
        if C3 is off or none were found: give up
        make them explicit in q (lower = upper = value) and loop again
    otherwise (pivot limit): give up
x := map(point); check x against all bounds and disequalities; install x
```

Every round of the loop removes at least one variable of `q`, so it ends.

### Statistics

Printed with `--stats-internal`, under `theory::arith::cube::`:

| statistic | meaning |
|---|---|
| `calls` | times the test was called |
| `success` | times it produced the model |
| `notApplicable` | not called: the problem is not pure integer, or is nonlinear |
| `skippedEquality` | skipped because of an equality (C3 off) |
| `noRoom` | the shifted bounds of some variable cross |
| `shiftedInfeasible` | the shifted system is infeasible |
| `shiftedUnknown` | pivot limit reached in C1 |
| `roundFailed` | rounded point violates a bound (should never happen) |
| `diseqFailed` | rounded point violates a disequality |
| `time` | total time of the test, including C2 and C3 |
| `impliedEq::calls`, `::rounds`, `::found`, `::unknown`, `::time` | C2: calls, strict systems solved, equalities found, pivot limit reached, time |
| `eqElim::calls`, `::equalities`, `::fresh`, `::infeasible`, `::unknown`, `::time` | C3: calls, equalities eliminated, fresh variables, no integer solution, step limit reached, time |

## 4. Code map

New files:

| file | content |
|---|---|
| `src/theory/arith/linear/cube/exact_simplex.{h,cpp}` | `ExactSimplex` |
| `src/theory/arith/linear/cube/unit_cube.{h,cpp}` | `CubeProblem`, C1 (`runUnitCubeTest`), C2 (`detectImpliedEqualities`) |
| `src/theory/arith/linear/cube/equality_elimination.{h,cpp}` | C3 (`eliminateEqualities`, `composeMaps`, `applyMap`) |
| `test/unit/theory/arith_exact_simplex_white.cpp` | 9 unit tests |
| `test/unit/theory/arith_unit_cube_white.cpp` | 11 unit tests |
| `test/unit/theory/arith_equality_elimination_white.cpp` | 10 unit tests |
| `test/regress/cli/regress0/arith/cube-test-*.smt2` | 16 regressions |
| `project/scripts/` | benchmark and mutation scripts (§9) |
| `project/results/` | benchmark and mutation results |

Changed files:

| file | change |
|---|---|
| `src/options/arith_options.toml` | the four options |
| `src/theory/arith/linear/theory_arith_private.{h,cpp}` | statistics; `cubeTestApplicable`, `cubeHasEquality`, `cubeSnapshot`, `cubeSatisfiesAll`, `cubeInstall`, `cubeImpliedEqualities`, `cubeEliminateEqualities`, `unitCubeTest`; the call in `postCheck` |
| `src/CMakeLists.txt`, `test/unit/theory/CMakeLists.txt`, `test/regress/cli/CMakeLists.txt` | register the new files |

The `cube/` code depends only on cvc5's number types and `Tableau`; it does not
know about the solver. Everything that touches cvc5's state is in
`theory_arith_private.cpp`.

## 5. What has been done and checked

1. **Rebase onto 1.4.1** of Jorge's first version (options, statistics, call
   site), replacing its in-place solve with `ExactSimplex`.
2. **`ExactSimplex`** with strict bounds, explanations and a pivot limit.
3. **C1**, with the check against all constraints (including disequalities)
   and the direct installation of the point.
4. **C2**, following Lemma 6.
5. **Proofs:** the guard that disabled the test with proofs was removed; the
   debug sweep runs with `--check-proofs`.
6. **Equality check before the test** (skip if C3 is off).
7. **C3**, Griggio's method decoupled from the solver, and the loop between
   C1, C2 and C3.
8. **Tests:** unit tests (including randomized ones that compare against
   brute force), regressions, a mutation check, debug sweeps.
9. **Benchmark scripts** (download, sampling, running, summarizing) and a first
   benchmark run with model re-checks.

Checks done so far, all passing:

| check | what it shows |
|---|---|
| 30 unit tests (debug build) | each component is correct in isolation: the shifts, rounding, explanations, implied equalities, elimination (on 200 random systems compared with brute force, the reduced problem has exactly the same integer solutions) |
| 16 regressions | the components behave as expected inside cvc5 (§9.2) |
| mutation check, 6 mutants | the tests catch deliberate bugs in C1, C2 and the simplex (§9.4) |
| debug sweep with skip on equalities (140 files) | same answers as plain cvc5, `--check-models --check-proofs` pass |
| debug sweep with C1+C2+C3 (144 files) | same answers, checks pass; 14 successes, 31 eliminations |
| benchmark, 150 files (§6) | 0 wrong answers against SMT-LIB status and plain cvc5; all 351 sat models pass `--check-models` |

## 6. Results so far

Run on 30 random files from each of the proposal's families (CAV 2009,
dillig, slacks, prime-cone, 20180326-Bromberger), 60 s, optimized build. Full
data, setup and caveats: [`results/sample30-t60/`](results/sample30-t60/README.md).

| config | options | solved | timeouts | PAR-2 (s) | time on the 132 common files (s) |
|---|---|---|---|---|---|
| B0 | none | 132 | 18 | 2505.5 | 345.5 |
| B1 | C1 | 140 | 10 | 1210.8 | 9.5 |
| B1+C2 | C1, C2 | 140 | 10 | 1211.3 | 9.9 |
| B2 | C1, C2, C3 | 140 | 10 | 1212.3 | 10.9 |

- **RQ1 (first answer):** the test helps a lot on these families. On sat
  problems with room it usually succeeds on its first call, turning several
  60 s timeouts into answers in under 0.3 s. Where it cannot help (unsat
  problems, prime-cone), it costs little in total (about 3.5 s over 150
  files for C1).
- **RQ2 (first answer):** on this sample C2 and C3 work (C3 eliminated 345
  equalities in 137 calls) but solve nothing more: after the elimination the
  shifted system was still infeasible every time. On prime-cone they only add
  time (2.1 s → 3.4 s). The proposal's targeted set (iii) is needed to answer
  RQ2 properly.
- The 10 files no configuration solves: the test runs on all of them (3 to 389
  times) and always fails, almost always because the shifted system is
  infeasible (on two files sometimes for lack of room).

Caveats: 150 of ~1,900 files, 60 s instead of 120 s, one run per file, and B0
is our binary with the options off rather than a separate cvc5 1.4.1 build.

## 7. Known limitations and open questions

- **Pure integer problems only** (by scope). Mixed problems are skipped.
- **Explicit equalities only, before C2.** `cubeHasEquality` sees an equality
  only when a variable's two bounds are equal. Implied equalities (such as
  `x ≤ y ≤ z ≤ x`) are found only by C2, after the shifted system fails.
- **Disequalities in pure QF_LIA.** By default cvc5 rewrites equalities in
  these logics (`--arith-rewrite-equalities`), so disequalities usually reach
  the arithmetic solver as Boolean structure instead. The disequality check
  is exercised with `--no-arith-rewrite-equalities`
  (`cube-test-diseq-rounded.smt2`).
- **Repeated calls.** The test runs at every full-effort check where the
  relaxation is not integral, and copies the whole problem each time. On
  problems where it keeps failing (prime-cone, the 10 timeouts) this is
  repeated work. The paper's filters, or calling it less often, are RQ3.
- **C3 overhead without gains** so far (see §6).
- **C3 was not part of the mutation check** (it was written afterwards).
- **The benchmark's unsat answers were not proof-checked** (only the debug
  sweep ran `--check-proofs`), and no other solver (Z3) was used yet as a
  cross-check.
- `--tlimit` ends cvc5 with "interrupted by timeout" and an abort (exit 134).
  This happens without our options as well; it is not a bug of ours, but
  scripts must not count it as a crash.

## 8. Plan for what comes next

Ordered by priority. Milestones from the proposal: progress review Nov 9–13,
B2 runs and RQ3 Nov 14–22, report and artifact Nov 23–29.

### 8.1 Review and commit (now)

1. Both of us read the changes (§4) and the tests.
2. Split them into reviewable commits, e.g.: `ExactSimplex` + tests; C1 +
   options + statistics + regressions; C2; equality check; C3; project scripts
   and results.
3. Run the full test suite on the final state before committing (§9.6).

### 8.2 Close the testing gaps (before the progress review)

1. Mutation check of C3 (new mutants, §9.4).
2. Full cvc5 regression suite (`ctest`) on the debug build, options off and
   with the cube options on.
3. Proof check of the benchmark's unsat answers (`--check-proofs`).
4. Z3 as an external cross-check on all benchmark answers (proposal §3).

### 8.3 Evaluation (proposal §3)

1. **Baselines:** build a separate, unmodified cvc5 1.4.1 for B0, to remove
   any doubt that "options off" is identical.
2. **Configurations:** B0 (default, brab on), B0− (`--no-arith-brab`), B1 and
   B1− (B0, B0− with C1), B2 (C1–C3). Externally: Z3, and SPASS-SATT if its
   binary still runs.
3. **Benchmark sets:**
   - (i) all files of CAV 2009, dillig, slacks, prime-cone, plus the paper's
     rotate family, regenerated from its definition (generator to write);
   - (ii) 20180326-Bromberger, complete;
   - (iii) RQ2 targets: variants of (i) whose equalities are implied, have
     non-unit coefficients, or sit under Boolean structure (generator to
     write), plus the SMT-LIB files where C1 fails with an infeasible shifted
     system (reported separately, since our own tool selects them);
   - (iv) 1,000 other QF_LIA files drawn with a fixed seed, at 60 s, to
     measure overhead.
4. **Timeouts:** 120 s for (i)–(iii), 60 s for (iv), as in the proposal.
   Fewer parallel jobs than cores, and repeat timings where differences are
   small.
5. **Measures:** solved instances, PAR-2, scatter plots against B0, time in
   the test, success rate, equalities found and eliminated (all already in
   the CSV produced by `run_bench.py`; plots to add to `summarize.py` or a
   notebook).

### 8.4 RQ3: when to call the test

1. Compare calling it at every full-effort check (current behaviour) with
   the paper's filters (e.g. skipping problems with 0/1 variables) and with
   calling it less often (e.g. only once per problem, or only after a check
   where the bounds changed). Each would be a new option, off by default, so
   the current behaviour stays the default of the test.
2. Interaction with brab: compare B1 against B1−, and possibly the
   alternative installation through brab lemmas (decision (b)).
3. Implement only what the measurements call for.

### 8.5 Report and artifact

LIPIcs report; artifact with README, the scripts, and raw data
(`project/results/` already follows this layout).

## 9. Testing plan

Testing has layers, from small and fast to large and slow. Every change
should pass layers 1–3; layers 4–6 before committing and before the review;
layer 7 is the evaluation itself.

### 9.1 Unit tests (seconds)

Test each component on its own, without the solver, in the debug build.

| binary | covers |
|---|---|
| `arith_exact_simplex_white` | feasible and infeasible systems, strict bounds, explanations (checked to be real Farkas conflicts), pivot limit, bounds changed between calls; 9 tests including random systems |
| `arith_unit_cube_white` | the shifts, the paper's example, equalities and no room, implied equalities (cycle, variable bounds, none), pivot limit; random tests: a cube that fits is found, every point found is a solution, implied equalities are real |
| `arith_equality_elimination_white` | unit coefficients, decomposition with fresh variables, the infeasible cases (gcd, fractional value, constant out of bounds), rational coefficients, step limit, composition of maps, elimination followed by C1; 200 random systems compared with brute force |

New code gets a unit test with a hand-computed example, and a random test
compared with brute force where possible.

### 9.2 Regressions (a minute)

Small SMT-LIB files run by cvc5's `ctest`, each with the options it needs and
the expected answer; most also check the statistics (so they fail if, for
example, the test stops succeeding) and `--check-models`.

| regression | expected behaviour |
|---|---|
| `cube-test-unbounded` | infinite lattice width: the test succeeds |
| `cube-test-paper-example` | the paper's example: no cube fits, branching finds (1, 3) |
| `cube-test-unsat` | relaxation sat but problem unsat: the test must not interfere |
| `cube-test-bool` | Boolean structure: several calls |
| `cube-test-diseq`, `cube-test-diseq-rounded` | disequalities; the rounded point violates one and is rejected |
| `cube-test-equality` | an equality: test skipped (`skippedEquality`) |
| `cube-test-narrow` | no equality but too narrow: `noRoom` |
| `cube-test-implied-cycle`, `-bounds`, `-bool`, `-unsat` | C2 finds the implied equalities; never a conflict |
| `cube-test-eqelim` | C3 removes `x + y = 3`, then the test succeeds |
| `cube-test-eqelim-decompose` | `3x + 5y = 7`: elimination with fresh variables |
| `cube-test-eqelim-implied` | C2 finds equalities, C3 removes them, the test succeeds |
| `cube-test-eqelim-unsat` | equalities without non-negative solutions: C3 must not interfere |

### 9.3 Debug sweep (tens of minutes)

Run cvc5's existing arithmetic regressions (pure integer ones are the
relevant part) in the debug build with all cube options on, plus
`--check-models --check-proofs`, and compare the answers with the same run
without our options. Any different answer, failed check or assertion is a
bug. Each file must be run with its own `COMMAND-LINE` options (some need
e.g. `--sat-solver=minisat`), otherwise the comparison is meaningless.

### 9.4 Mutation check (an hour per mutant set)

Shows that the tests would catch real bugs: insert one deliberate bug, rebuild,
run layers 1–2, check that something fails, restore. Always in a separate git
worktree, never in the main checkout:

```
git worktree add --detach ../cvc5-mutation
# copy the uncommitted changes into it, configure a debug build in ../cvc5-mutation/build
project/scripts/mutate.sh ../cvc5-mutation /some/output/dir
```

Done so far (results in [`results/mutation/`](results/mutation/summary.txt)):

| mutant | bug | caught by |
|---|---|---|
| M1 | variable shift ¼ instead of ½ | `shift` unit test, `cube-test-diseq-rounded` |
| M2 | row shift ¼‖a‖₁ | `shift` unit test, assertion in C1 |
| M3 | round down instead of to nearest | assertion in C1, a regression |
| M4 | C2 treats non-strict bounds as equalities | random C2 test |
| M5 | disequality check skipped | `cube-test-diseq-rounded` |
| M6 | explanation missing a bound | 8 simplex tests, C2 assertion, 2 regressions |

To do: mutants for C3 (wrong sign in a substitution, floor instead of the
correct quotient in a decomposition, skipping the gcd check, a wrong constant
in the map, skipping the bound check of constant expressions) and for the
C1–C2–C3 loop (not making implied equalities explicit). Each must be caught by
a test; if one is not, add the missing test.

### 9.5 Benchmarks with cross-checks (hours)

On the optimized build, with `project/scripts/run_bench.py` and
`summarize.py`:

- every answer compared with the SMT-LIB status and with B0 (and later Z3);
  any disagreement is a bug (`summarize.py` lists them as "wrong answers");
- every `sat` answer of our configurations re-run with `--check-models`;
- every `unsat` answer re-run with `--check-proofs` (to do);
- the statistics show where the test fails and why.

### 9.6 Full test suite (an hour)

Before committing: the whole `ctest` suite of the debug build, to make sure
nothing outside the cube code changed (with the options off, nothing
should).

### 9.7 What counts as a bug

- any wrong answer (against status, B0 or Z3);
- any failed `--check-models` or `--check-proofs`;
- any assertion failure or crash in the debug build (except the abort on
  `--tlimit`, which plain cvc5 also does);
- `roundFailed` greater than 0 (the paper's theorem says it cannot happen).

## 10. How to build and run

Builds (from the repository root):

```
./configure.sh debug --name=build-debug --auto-download   # assertions, unit tests
./configure.sh unrestricted --name=build-release --auto-download   # optimized
make -C build-debug -j2 && make -C build-release -j2
```

Run cvc5 with the components:

```
build-release/bin/cvc5 --arith-cube-test --arith-cube-implied-eq --arith-cube-eq-elim \
  --stats-internal file.smt2 2>&1 | grep -E '^(sat|unsat|unknown)$|cube::'
```

Add `--check-models` to verify a sat answer; `-t arith::cube` (debug build)
prints a trace of what the test does.

Tests:

```
build-debug/bin/test/unit/theory/arith_exact_simplex_white
build-debug/bin/test/unit/theory/arith_unit_cube_white
build-debug/bin/test/unit/theory/arith_equality_elimination_white
cd build-debug && ctest -R cube-test
```

Benchmarks:

```
project/scripts/fetch_benchmarks.sh ~/cube-bench          # download and extract
python3 project/scripts/run_bench.py --binary build-release/bin/cvc5 \
  --bench-dir ~/cube-bench/QF_LIA --out results.csv \
  --sample 30 --seed 1 --timeout 60 --jobs 2
python3 project/scripts/summarize.py results.csv --timeout 60
```

`run_bench.py` can be interrupted and resumed (it skips runs already in the
CSV), and takes other configurations with `--config NAME="OPTIONS"`.

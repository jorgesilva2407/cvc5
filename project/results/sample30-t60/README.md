# Benchmark run: 30 files per family, 60 s

Plain cvc5 against cvc5 with the unit cube test components, on a sample of the
SMT-LIB 2024 QF_LIA families from the proposal.

## Setup

- Binary: cvc5 1.4.1 with our changes, optimized build
  (`./configure.sh unrestricted`, CMake build type Production,
  `build-release/bin/cvc5`).
- Benchmarks: SMT-LIB 2024 QF_LIA (Zenodo record 11061097), fetched with
  `project/scripts/fetch_benchmarks.sh`. 30 files drawn at random (seed 1)
  from each of 20180326-Bromberger, CAV_2009_benchmarks, dillig, prime-cone
  and slacks: 150 files (`files.txt`). Paths are relative to the directory the
  benchmarks were fetched into.
- Limits: 60 s wall clock (`timeout --signal=KILL`), 1200 MB address space,
  2 runs in parallel, one run per file and configuration.
- Configurations (every run also has `--stats-internal`):

  | name  | options                                                          |
  |-------|------------------------------------------------------------------|
  | B0    | none (plain cvc5: all new options are off by default)            |
  | B1    | `--arith-cube-test`                                              |
  | B1+C2 | `--arith-cube-test --arith-cube-implied-eq`                      |
  | B2    | `--arith-cube-test --arith-cube-implied-eq --arith-cube-eq-elim` |

- Command:

  ```
  python3 project/scripts/run_bench.py --binary build-release/bin/cvc5 \
    --bench-dir ~/cube-bench/QF_LIA --out results.csv \
    --sample 30 --seed 1 --timeout 60 --jobs 2 --mem-mb 1200
  python3 project/scripts/summarize.py results.csv --timeout 60 > summary.txt
  ```

## Files

- `results.csv`: one row per file and configuration (600 rows): answer,
  expected status from the file, exit code, timeout flag, wall and CPU time,
  peak memory, and the `theory::arith::cube::*` statistics as JSON.
- `files.txt`: the 150 sampled files.
- `summary.txt`: output of `summarize.py` (per family: solved, PAR-2, time on
  the files all configurations solve, wrong answers, cube statistics; then the
  files solved differently from B0).
- `sample.md`: a few files per family side by side.
- `model-check.txt`: every sat answer of B1, B1+C2 and B2 re-run with
  `--check-models` (config, answer, exit code, file). All 351 are ok.
- `timeouts.txt`, `timeouts-stats.txt`: the 10 files every configuration
  times out on, and the cube statistics of a B2 run on them.

## Results

| config | solved | timeouts | PAR-2 (s) | time on the 132 common files (s) |
|--------|--------|----------|-----------|----------------------------------|
| B0     | 132    | 18       | 2505.5    | 345.5                            |
| B1     | 140    | 10       | 1210.8    | 9.5                              |
| B1+C2  | 140    | 10       | 1211.3    | 9.9                              |
| B2     | 140    | 10       | 1212.3    | 10.9                             |

- 0 wrong answers (against the expected status and against B0); all 351 sat
  models of our configurations pass `--check-models`.
- 8 files time out with B0 and are solved by all our configurations in
  0.1–0.3 s (5 Bromberger, 3 slacks).
- C2 and C3 solve no extra files on this sample. C3 removes 345 equalities in
  137 calls, but the shifted system stays infeasible afterwards. On prime-cone
  they only add time (2.1 s → 2.7 s → 3.4 s).
- The 10 common timeouts: the test runs on all of them (3 to 389 times) and
  never succeeds (shifted system infeasible, sometimes no room).

## Caveats

- 60 s limit instead of the proposal's 120 s; a 150-file sample, not all
  ~1,900 files of these families.
- One run per file with 2 in parallel: small time differences are noise.
- B0 is our binary with the options off, not a separately built pristine
  cvc5 1.4.1.

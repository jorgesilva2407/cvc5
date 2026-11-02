# Sample of the results

Three files per family (random, seed 7); where a family has a file that B0
times out on but our configurations solve, one of those replaces a random
file. Cells: answer and wall-clock time (t/o = 60 s limit).

Stats: calls = cube test runs, ok = successes, shInf = shifted system
infeasible, elimEq = equalities removed by C3.

## 20180326-Bromberger

| file | expected | B0 | B1 | B1+C2 | B2 | B2 cube stats |
|---|---|---|---|---|---|---|
| v25_problem_2__035 | sat | t/o 60.00s | sat 0.24s | sat 0.24s | sat 0.23s | calls=1 ok=1 |
| v10_problem__021 | unsat | unsat 0.06s | unsat 0.02s | unsat 0.02s | unsat 0.02s | test not run |
| c50_problem__005 | sat | t/o 60.00s | sat 0.07s | sat 0.05s | sat 0.06s | calls=1 ok=1 |

## CAV_2009_benchmarks

| file | expected | B0 | B1 | B1+C2 | B2 | B2 cube stats |
|---|---|---|---|---|---|---|
| problem__017 | sat | sat 0.44s | sat 0.07s | sat 0.07s | sat 0.06s | calls=1 ok=1 |
| problem__005 | sat | sat 0.15s | sat 0.03s | sat 0.03s | sat 0.03s | calls=1 ok=1 |
| problem__003 | sat | sat 0.23s | sat 0.02s | sat 0.02s | sat 0.02s | calls=1 ok=1 |

## dillig

| file | expected | B0 | B1 | B1+C2 | B2 | B2 cube stats |
|---|---|---|---|---|---|---|
| 45-16 | sat | sat 1.14s | sat 0.08s | sat 0.08s | sat 0.08s | calls=1 ok=1 |
| 35-25 | sat | sat 1.03s | sat 0.08s | sat 0.08s | sat 0.08s | calls=1 ok=1 |
| 15-18 | sat | sat 0.06s | sat 0.04s | sat 0.03s | sat 0.03s | calls=1 ok=1 |

## prime-cone

| file | expected | B0 | B1 | B1+C2 | B2 | B2 cube stats |
|---|---|---|---|---|---|---|
| prime_cone_sat_5 | sat | sat 0.03s | sat 0.01s | sat 0.01s | sat 0.01s | calls=1 ok=1 |
| prime_cone_unsat_13 | unsat | unsat 0.09s | unsat 0.12s | unsat 0.15s | unsat 0.20s | calls=27 shInf=27 elimEq=15 |
| prime_cone_sat_13 | sat | sat 0.04s | sat 0.02s | sat 0.02s | sat 0.02s | calls=1 ok=1 |

## slacks

| file | expected | B0 | B1 | B1+C2 | B2 | B2 cube stats |
|---|---|---|---|---|---|---|
| 45-5 | sat | sat 2.27s | sat 0.04s | sat 0.04s | sat 0.08s | calls=1 ok=1 |
| 30-8 | sat | sat 0.03s | sat 0.03s | sat 0.03s | sat 0.03s | test not run |
| 30-28 | sat | t/o 60.00s | sat 0.19s | sat 0.20s | sat 0.20s | calls=1 ok=1 |

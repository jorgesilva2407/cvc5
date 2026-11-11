#!/bin/bash
# Equality-order experiment, configuration C2first+C1: cvc5 with implied-equality detection (C2) before the unit cube test (C1); the test is skipped if an equality is found.
#
# usage: eq_order_c2first.sh CVC5 WORKDIR
#   (TIMEOUT, JOBS and MEM_MB as in eq_order_run.sh; default 300 s, 1, 4000)
exec "$(dirname "$0")/eq_order_run.sh" "C2first+C1" "--arith-cube-test --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first" "$@"

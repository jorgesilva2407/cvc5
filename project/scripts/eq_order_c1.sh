#!/bin/bash
# Equality-order experiment, configuration C1: cvc5 with the unit cube test (C1).
#
# usage: eq_order_c1.sh CVC5 WORKDIR
#   (TIMEOUT, JOBS and MEM_MB as in eq_order_run.sh; default 300 s, 1, 4000)
exec "$(dirname "$0")/eq_order_run.sh" "C1" "--arith-cube-test" "$@"

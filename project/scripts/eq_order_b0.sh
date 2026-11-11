#!/bin/bash
# Equality-order experiment, configuration B0: plain cvc5.
#
# usage: eq_order_b0.sh CVC5 WORKDIR
#   (TIMEOUT, JOBS and MEM_MB as in eq_order_run.sh; default 300 s, 1, 4000)
exec "$(dirname "$0")/eq_order_run.sh" "B0" "" "$@"

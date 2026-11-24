#!/bin/bash
# The configurations of the report experiment, sourced by report_run.sh and
# report_all.sh. NAME -> cvc5 options. REPORT_ORDER is the order in which
# report_all.sh starts them.
#
#   B0              plain cvc5 1.4.1 (brab on, the default)
#   B0-nobrab       plain cvc5 without brab              (B0- in the proposal)
#   C1              the unit cube test                   (B1)
#   C1-nobrab       the test without brab                (B1-)
#   C1+C2first      implied equalities detected before the test, which is
#                   skipped if there are any (detect-first, without C3)
#   C1+C2first+C3   implied equalities detected before the test and
#                   eliminated, with the explicit ones; then the test on the
#                   reduced problem (detect-first with C3, the paper's order)

CUBE="--arith-cube-test"
C2FIRST="$CUBE --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first"

declare -A REPORT_CONFIGS=(
  ["B0"]=""
  ["B0-nobrab"]="--no-arith-brab"
  ["C1"]="$CUBE"
  ["C1-nobrab"]="$CUBE --no-arith-brab"
  ["C1+C2first"]="$C2FIRST"
  ["C1+C2first+C3"]="$C2FIRST --arith-cube-eq-elim"
)
REPORT_ORDER=(B0 B0-nobrab C1 C1-nobrab C1+C2first C1+C2first+C3)

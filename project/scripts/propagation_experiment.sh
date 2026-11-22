#!/bin/bash
# Propagation experiment: how much sending the implied equalities found by
# C2 to cvc5 helps. Runs these configurations at the same time (one process
# each, see eq_order_run.sh), on the instances of the equality-order
# experiment (5 families and their +ie/+ie2 variants), and waits for them:
#
#   B0            plain cvc5
#   C1            the unit cube test
#   C1+C2         and the implied equalities, detected after the test fails
#                 (the control: the same pipeline without propagation)
#   C1+C2+P       and the implied equalities propagated as lemmas
#   C1+C2+P+skip  and also when the test is skipped for an explicit equality
#
# usage: propagation_experiment.sh CVC5 WORKDIR
#
# Environment (optional):
#   SOURCE   WORKDIR of the equality-order experiment (e.g. ~/cube-eq): its
#            benchmarks and instance list are reused instead of downloading
#            them again, so the instances are exactly the same
#   TIMEOUT, JOBS, MEM_MB as in eq_order_run.sh (default 300 s, 1 job per
#            configuration, 4000 MB)
#
# Logs: WORKDIR/logs/NAME.log. Report:
#   report_experiments.py propagation WORKDIR/results --timeout 300
set -euo pipefail
CVC5=${1:?usage: propagation_experiment.sh CVC5 WORKDIR}
WORK=${2:?usage: propagation_experiment.sh CVC5 WORKDIR}
HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$WORK/logs"
if [ -n "${SOURCE:-}" ] && [ ! -f "$WORK/files.tsv" ]; then
  ln -sfn "$(realpath "$SOURCE")/QF_LIA" "$WORK/QF_LIA"
  cp "$SOURCE/files.tsv" "$WORK/files.tsv"
fi
CUBE="--arith-cube-test"
C2="$CUBE --arith-cube-implied-eq"
P="$C2 --arith-cube-propagate-eq"
declare -A CONFIGS=(
  ["B0"]=""
  ["C1"]="$CUBE"
  ["C1+C2"]="$C2"
  ["C1+C2+P"]="$P"
  ["C1+C2+P+skip"]="$P --arith-cube-propagate-eq-on-skip"
)
for name in "${!CONFIGS[@]}"; do
  JOBS=${JOBS:-1} "$HERE/eq_order_run.sh" "$name" "${CONFIGS[$name]}" "$CVC5" "$WORK" \
    > "$WORK/logs/$name.log" 2>&1 &
  echo "started $name (pid $!, log $WORK/logs/$name.log)"
done
wait
echo "all done; report with:"
echo "  python3 -I $HERE/report_experiments.py propagation $WORK/results --timeout ${TIMEOUT:-300}"

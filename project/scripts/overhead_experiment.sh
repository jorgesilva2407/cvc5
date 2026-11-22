#!/bin/bash
# Overhead experiment: what the unit cube test costs on QF_LIA problems it
# was not chosen for (proposal set (iv): 1000 random files from the other
# families, 60 s). Runs these configurations at the same time (one process
# each, see overhead_run.sh) and waits for them:
#
#   B0            plain cvc5
#   C1            the unit cube test
#   C1+C2+P+skip  the test with implied equalities and their propagation
#
# usage: overhead_experiment.sh CVC5 WORKDIR
#
# Environment (optional): ARCHIVE, COUNT, TIMEOUT, JOBS, MEM_MB as in
# overhead_run.sh (pass ARCHIVE=~/cube-eq/download/QF_LIA.tar.zst to reuse
# the archive already downloaded).
#
# Logs: WORKDIR/logs/NAME.log. Report:
#   report_experiments.py overhead WORKDIR/results --timeout 60
set -euo pipefail
CVC5=${1:?usage: overhead_experiment.sh CVC5 WORKDIR}
WORK=${2:?usage: overhead_experiment.sh CVC5 WORKDIR}
HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$WORK/logs"
P="--arith-cube-test --arith-cube-implied-eq --arith-cube-propagate-eq"
declare -A CONFIGS=(
  ["B0"]=""
  ["C1"]="--arith-cube-test"
  ["C1+C2+P+skip"]="$P --arith-cube-propagate-eq-on-skip"
)
for name in "${!CONFIGS[@]}"; do
  JOBS=${JOBS:-1} "$HERE/overhead_run.sh" "$name" "${CONFIGS[$name]}" "$CVC5" "$WORK" \
    > "$WORK/logs/$name.log" 2>&1 &
  echo "started $name (pid $!, log $WORK/logs/$name.log)"
done
wait
echo "all done; report with:"
echo "  python3 -I $HERE/report_experiments.py overhead $WORK/results --timeout ${TIMEOUT:-60}"

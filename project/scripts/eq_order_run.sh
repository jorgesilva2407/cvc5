#!/bin/bash
# Runs one configuration of the equality-order experiment. Called by
# eq_order_b0.sh, eq_order_c1.sh and eq_order_c2first.sh, which can run at
# the same time (each writes its own CSV; the benchmarks are prepared once).
#
# usage: eq_order_run.sh NAME "OPTIONS" CVC5 WORKDIR
#
# Environment (optional):
#   TIMEOUT  seconds per run (default 300)
#   JOBS     runs in parallel within this configuration (default 1)
#   MEM_MB   address space limit per run in MB (default 4000)
#
# Results: WORKDIR/results/NAME.csv (one row per instance, one column per
# statistic) and WORKDIR/results/NAME.setup.txt. Running the same command
# again resumes an interrupted run.
set -euo pipefail

NAME=${1:?usage: eq_order_run.sh NAME OPTIONS CVC5 WORKDIR}
OPTIONS=$2
CVC5=$(realpath "${3:?usage: eq_order_run.sh NAME OPTIONS CVC5 WORKDIR}")
WORK=$(mkdir -p "${4:?usage: eq_order_run.sh NAME OPTIONS CVC5 WORKDIR}" \
  && realpath "$4")
TIMEOUT=${TIMEOUT:-300}
JOBS=${JOBS:-1}
MEM_MB=${MEM_MB:-4000}
HERE=$(cd "$(dirname "$0")" && pwd)

# fails if the binary does not have the options (e.g. not built from this
# branch)
"$CVC5" $OPTIONS --version > /dev/null

# Download the benchmarks and choose the instances, once: the other
# configurations wait here until this is done.
(
  flock 9
  if [ ! -d "$WORK/QF_LIA/dillig" ]; then
    "$HERE/fetch_benchmarks.sh" "$WORK"
  fi
  if [ ! -f "$WORK/files.tsv" ]; then
    python3 -I "$HERE/prepare_eq_order.py" "$WORK/QF_LIA" "$WORK"
  fi
) 9> "$WORK/.prepare.lock"

mkdir -p "$WORK/results"
{
  echo "configuration: $NAME"
  echo "options: $OPTIONS"
  echo "binary: $CVC5"
  "$CVC5" --version | head -1
  echo "timeout: $TIMEOUT s, jobs: $JOBS, memory: $MEM_MB MB"
  echo "instances: $(wc -l < "$WORK/files.tsv") ($WORK/files.tsv)"
  echo "host: $(uname -a)"
  echo "cpu: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
  echo "started: $(date)"
} > "$WORK/results/$NAME.setup.txt"

python3 -I "$HERE/run_bench.py" --binary "$CVC5" \
  --file-list "$WORK/files.tsv" --out "$WORK/results/$NAME.csv" \
  --timeout "$TIMEOUT" --jobs "$JOBS" --mem-mb "$MEM_MB" \
  --config "$NAME=$OPTIONS"

echo "finished: $(date)" >> "$WORK/results/$NAME.setup.txt"
echo "done: $WORK/results/$NAME.csv"

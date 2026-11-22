#!/bin/bash
# Runs one configuration of the overhead experiment (proposal set (iv)): N
# random QF_LIA files from the families the unit cube test was not chosen
# for. Several configurations can run at the same time (each writes its own
# CSV; the instances are chosen once).
#
# usage: overhead_run.sh NAME "OPTIONS" CVC5 WORKDIR
#
# Environment (optional):
#   ARCHIVE  QF_LIA.tar.zst to draw from (default: downloaded into WORKDIR by
#            fetch_benchmarks.sh; pass the one of the equality-order
#            experiment, e.g. ~/cube-eq/download/QF_LIA.tar.zst, to reuse it)
#   COUNT    number of instances (default 1000)
#   TIMEOUT  seconds per run (default 60)
#   JOBS     runs in parallel within this configuration (default 1)
#   MEM_MB   address space limit per run in MB (default 4000)
#
# Results: WORKDIR/results/NAME.csv and WORKDIR/results/NAME.setup.txt.
# Running the same command again resumes an interrupted run.
set -euo pipefail

NAME=${1:?usage: overhead_run.sh NAME OPTIONS CVC5 WORKDIR}
OPTIONS=$2
CVC5=$(realpath "${3:?usage: overhead_run.sh NAME OPTIONS CVC5 WORKDIR}")
WORK=$(mkdir -p "${4:?usage: overhead_run.sh NAME OPTIONS CVC5 WORKDIR}" \
  && realpath "$4")
COUNT=${COUNT:-1000}
TIMEOUT=${TIMEOUT:-60}
JOBS=${JOBS:-1}
MEM_MB=${MEM_MB:-4000}
HERE=$(cd "$(dirname "$0")" && pwd)

# fails if the binary does not have the options (e.g. not built from this
# branch)
"$CVC5" $OPTIONS --version > /dev/null

# Choose and extract the instances, once: the other configurations wait here
# until this is done.
(
  flock 9
  if [ ! -f "$WORK/files.tsv" ]; then
    if [ -z "${ARCHIVE:-}" ]; then
      "$HERE/fetch_benchmarks.sh" "$WORK"
      ARCHIVE=$WORK/download/QF_LIA.tar.zst
    fi
    if [ ! -x "$WORK/venv/bin/python" ]; then
      python3 -m venv "$WORK/venv"
      "$WORK/venv/bin/pip" install -q zstandard
    fi
    "$WORK/venv/bin/python" -I "$HERE/prepare_random.py" "$(realpath "$ARCHIVE")" \
      "$WORK" "$COUNT"
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

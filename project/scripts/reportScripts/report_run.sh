#!/bin/bash
# Runs one configuration of the report experiment on all instances. Called by
# the report_<config>.sh scripts (one per configuration), which can run at the
# same time, e.g. each in its own tmux window: each writes its own CSV, and
# the benchmarks are downloaded and prepared only once.
#
# usage: report_run.sh NAME CVC5 WORKDIR
#   NAME is one of the configurations of report_configs.sh.
#
# Environment (optional):
#   TIMEOUT   seconds per run (default 300)
#   JOBS      runs in parallel within this configuration (default 1)
#   MEM_MB    address space limit per run in MB (default 4000)
#   RESULTS   where the results go (default: project/results/report_results
#             of this checkout)
#   VARIANTS  "one" (default): one variant per file, +ie or +ie2 (950 of
#             each); "all": both variants of every file; "none": only the
#             original files. Only used when the instance list is made (by
#             the first script to start); see prepare_report.py.
#
# WORKDIR can be the one of an earlier experiment (e.g. ~/cube-eq): the
# benchmarks already downloaded there are reused. The instance list and the
# variants of this experiment are kept apart from the earlier ones:
#   WORKDIR/report_files.tsv       the instances ("FAMILY<TAB>PATH")
#   WORKDIR/report_variants/       the +ie/+ie2 variants
# The results go to RESULTS (by default project/results/report_results):
#   RESULTS/NAME.csv        one row per instance
#   RESULTS/NAME.setup.txt  machine, options, start and end
# Running the same command again resumes an interrupted run.
set -euo pipefail

USAGE="usage: report_run.sh NAME CVC5 WORKDIR"
NAME=${1:?$USAGE}
CVC5=$(realpath "${2:?$USAGE}")
WORK=$(mkdir -p "${3:?$USAGE}" && realpath "$3")
TIMEOUT=${TIMEOUT:-300}
JOBS=${JOBS:-1}
MEM_MB=${MEM_MB:-4000}
VARIANTS=${VARIANTS:-one}
HERE=$(cd "$(dirname "$0")" && pwd)
# the shared scripts (run_bench.py, summarize.py, fetch_benchmarks.sh, ...)
SCRIPTS=$(dirname "$HERE")

source "$HERE/report_configs.sh"
if [ -z "${REPORT_CONFIGS[$NAME]+set}" ]; then
  echo "unknown configuration $NAME; known: ${REPORT_ORDER[*]}" >&2
  exit 1
fi
OPTIONS=${REPORT_CONFIGS[$NAME]}

# fails if the binary does not accept the options (e.g. not built from this
# branch, or built before detect-first was combined with C3)
"$CVC5" $OPTIONS --version > /dev/null

# Download the benchmarks and choose the instances, once: the other
# configurations wait here until this is done.
(
  flock 9
  if [ ! -d "$WORK/QF_LIA/dillig" ]; then
    "$SCRIPTS/fetch_benchmarks.sh" "$WORK"
  fi
  if [ ! -f "$WORK/report_files.tsv" ]; then
    python3 -I "$HERE/prepare_report.py" "$WORK/QF_LIA" "$WORK" \
      --variants "$VARIANTS"
  fi
) 9> "$WORK/.report-prepare.lock"

R=${RESULTS:-$SCRIPTS/../results/report_results}
mkdir -p "$R"
R=$(realpath "$R")
{
  echo "configuration: $NAME"
  echo "options: $OPTIONS"
  echo "binary: $CVC5"
  "$CVC5" --version | head -1
  echo "timeout: $TIMEOUT s, jobs: $JOBS, memory: $MEM_MB MB"
  echo "instances: $(wc -l < "$WORK/report_files.tsv") ($WORK/report_files.tsv; variants: see report_files.tsv, VARIANTS=$VARIANTS when made)"
  echo "host: $(uname -a)"
  echo "cpu: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
  echo "started: $(date)"
} > "$R/$NAME.setup.txt"

python3 -I "$SCRIPTS/run_bench.py" --binary "$CVC5" \
  --file-list "$WORK/report_files.tsv" --out "$R/$NAME.csv" \
  --timeout "$TIMEOUT" --jobs "$JOBS" --mem-mb "$MEM_MB" \
  --config "$NAME=$OPTIONS"

echo "finished: $(date)" >> "$R/$NAME.setup.txt"
echo "done: $R/$NAME.csv"

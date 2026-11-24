#!/bin/bash
# Report experiment: starts every configuration of report_configs.sh at the
# same time, each as its own background process (see report_run.sh), and
# waits for them. The alternative is one tmux window per configuration, each
# running its report_<config>.sh (see the README).
#
# usage: report_all.sh CVC5 WORKDIR [NAME ...]
#   With NAMEs, only those configurations are started.
#   (TIMEOUT, JOBS, MEM_MB and VARIANTS as in report_run.sh)
#
# Logs: WORKDIR/report_logs/NAME.log. Results: RESULTS (default
# project/results/report_results). When they are done: report_summary.sh
set -euo pipefail
USAGE="usage: report_all.sh CVC5 WORKDIR [NAME ...]"
CVC5=${1:?$USAGE}
WORK=$(mkdir -p "${2:?$USAGE}" && realpath "$2")
shift 2
HERE=$(cd "$(dirname "$0")" && pwd)
source "$HERE/report_configs.sh"
NAMES=("$@")
[ ${#NAMES[@]} -gt 0 ] || NAMES=("${REPORT_ORDER[@]}")
mkdir -p "$WORK/report_logs"
for name in "${NAMES[@]}"; do
  "$HERE/report_run.sh" "$name" "$CVC5" "$WORK" \
    > "$WORK/report_logs/$name.log" 2>&1 &
  echo "started $name (pid $!, log $WORK/report_logs/$name.log)"
done
wait
echo "all done; summarize with: $HERE/report_summary.sh"

#!/bin/bash
# Summarizes the report experiment: merges the CSVs of all configurations in
# RESULTS (default project/results/report_results) into all.csv, and writes
#   summary.txt  solved, PAR-2, wrong answers and test statistics per
#                family and configuration (summarize.py, against B0)
# Can be run while configurations are still running (it summarizes what is
# there).
#
# usage: report_summary.sh
#   TIMEOUT (default 300) must be the one the runs used; RESULTS as in
#   report_run.sh.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
# the shared scripts (run_bench.py, summarize.py, fetch_benchmarks.sh, ...)
SCRIPTS=$(dirname "$HERE")
R=${RESULTS:-$SCRIPTS/../results/report_results}
T=${TIMEOUT:-300}
python3 -I - "$R" <<'PY'
import csv, glob, os, sys
r = sys.argv[1]
parts = sorted(p for p in glob.glob(os.path.join(r, "*.csv"))
               if os.path.basename(p) != "all.csv")
rows, fields = [], None
for p in parts:
    with open(p) as fh:
        reader = csv.DictReader(fh)
        fields = fields or reader.fieldnames
        rows += list(reader)
with open(os.path.join(r, "all.csv"), "w", newline="") as out:
    w = csv.DictWriter(out, fields)
    w.writeheader()
    w.writerows(rows)
print(f"{len(rows)} rows from {', '.join(map(os.path.basename, parts))}")
PY
python3 -I "$SCRIPTS/summarize.py" "$R/all.csv" --timeout "$T" > "$R/summary.txt"
echo "summary: $R/summary.txt"

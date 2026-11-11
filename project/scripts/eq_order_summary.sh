#!/bin/bash
# Merges the results of the equality-order experiment (one CSV per
# configuration) into WORKDIR/results/all.csv and summarizes them into
# WORKDIR/results/summary.txt. Can be run while configurations are still
# running (it summarizes what is there).
#
# usage: eq_order_summary.sh WORKDIR
set -euo pipefail
WORK=${1:?usage: eq_order_summary.sh WORKDIR}
HERE=$(cd "$(dirname "$0")" && pwd)
R=$WORK/results
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
python3 -I "$HERE/summarize.py" "$R/all.csv" --timeout "${TIMEOUT:-300}" \
  > "$R/summary.txt"
echo "summary: $R/summary.txt"

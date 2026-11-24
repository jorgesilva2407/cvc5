#!/bin/bash
# Report experiment, configuration C1+C2first: implied-equality detection, then the test if none is found.
# (options in report_configs.sh).
#
# usage: report_c2first.sh CVC5 WORKDIR
#   (TIMEOUT, JOBS, MEM_MB and VARIANTS as in report_run.sh; default 300 s,
#   1 job, 4000 MB, one variant per file)
exec "$(dirname "$0")/report_run.sh" "C1+C2first" "$@"

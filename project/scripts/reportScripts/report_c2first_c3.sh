#!/bin/bash
# Report experiment, configuration C1+C2first+C3: detect the implied
# equalities, eliminate them (with the explicit ones), then run the test on
# the reduced problem (the paper's order, Sec. 6). Options in
# report_configs.sh.
#
# usage: report_c2first_c3.sh CVC5 WORKDIR
#   (TIMEOUT, JOBS, MEM_MB and VARIANTS as in report_run.sh; default 300 s,
#   1 job, 4000 MB, one variant per file)
exec "$(dirname "$0")/report_run.sh" "C1+C2first+C3" "$@"

#!/bin/bash
# Downloads the SMT-LIB 2024 QF_LIA benchmarks (Zenodo record 11061097),
# checks the archive, and extracts the families of the proposal's sets (i)
# and (ii) into OUTDIR/QF_LIA/<family>.
#
# usage: fetch_benchmarks.sh OUTDIR
set -euo pipefail

OUTDIR=${1:?usage: fetch_benchmarks.sh OUTDIR}
URL=https://zenodo.org/api/records/11061097/files/QF_LIA.tar.zst/content
MD5=d71191111a0d80d6558a8784058d081c
HERE=$(cd "$(dirname "$0")" && pwd)

mkdir -p "$OUTDIR/download" "$OUTDIR/QF_LIA"
ARCHIVE=$OUTDIR/download/QF_LIA.tar.zst
if [ ! -f "$ARCHIVE" ]; then
  curl -L --fail -o "$ARCHIVE" "$URL"
fi
echo "$MD5  $ARCHIVE" | md5sum -c -

# The archive is zstd-compressed; extraction uses the Python zstandard
# package, installed into a virtual environment next to the data.
if [ ! -x "$OUTDIR/venv/bin/python" ]; then
  python3 -m venv "$OUTDIR/venv"
  "$OUTDIR/venv/bin/pip" install -q zstandard
fi
"$OUTDIR/venv/bin/python" -I "$HERE/extract_families.py" "$ARCHIVE" \
  "$OUTDIR/QF_LIA"

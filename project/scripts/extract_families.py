"""Extract benchmark families from the SMT-LIB QF_LIA archive.

usage: extract_families.py ARCHIVE OUTDIR [FAMILY ...]

ARCHIVE is QF_LIA.tar.zst from the SMT-LIB 2024 release. Each family is
extracted to OUTDIR/<family>; a family matches if its directory name contains
one of the given names (case-insensitive). Without names, the families of the
proposal's sets (i) and (ii) are extracted. Prints the number of files of
every family in the archive.
"""
import os
import sys
import tarfile

import zstandard

DEFAULT_FAMILIES = ("cav_2009", "dillig", "slacks", "prime-cone", "bromberger")

archive, outdir = sys.argv[1], sys.argv[2]
families = tuple(f.lower() for f in sys.argv[3:]) or DEFAULT_FAMILIES
seen = {}
with open(archive, "rb") as fh:
    reader = zstandard.ZstdDecompressor().stream_reader(fh)
    with tarfile.open(fileobj=reader, mode="r|") as tar:
        for m in tar:
            parts = m.name.split("/")
            # non-incremental/QF_LIA/<family>/...
            family = parts[2] if len(parts) > 2 else ""
            seen[family] = seen.get(family, 0) + (1 if m.isfile() else 0)
            if not m.isfile() or not any(f in family.lower() for f in families):
                continue
            if ".." in parts or m.name.startswith("/"):
                continue
            dest = os.path.join(outdir, *parts[2:])
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            with open(dest, "wb") as out:
                out.write(tar.extractfile(m).read())
for family, n in sorted(seen.items()):
    print(n, family)

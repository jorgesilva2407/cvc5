"""Choose the instances of the overhead experiment (proposal set (iv)).

usage: prepare_random.py ARCHIVE WORKDIR [N]

ARCHIVE is QF_LIA.tar.zst (as downloaded by fetch_benchmarks.sh). Draws N
files (default 1000) at random, with a fixed SEED, from the QF_LIA families
other than those of the proposal's sets (i) and (ii), i.e. those the unit
cube test was not chosen for. Only the chosen files are extracted, to
WORKDIR/QF_LIA-random/<family>/..., and their list is written to
WORKDIR/files.tsv (lines "FAMILY<TAB>PATH"), which run_bench.py reads with
--file-list.
"""
import os
import random
import sys
import tarfile

import zstandard

SEED = 1
# the families of sets (i) and (ii), matched as in extract_families.py
EXCLUDED = ("cav_2009", "dillig", "slacks", "prime-cone", "bromberger")


def members(archive):
    """Yields (member, tar) for every file of the archive, in order."""
    with open(archive, "rb") as fh:
        reader = zstandard.ZstdDecompressor().stream_reader(fh)
        with tarfile.open(fileobj=reader, mode="r|") as tar:
            for m in tar:
                if m.isfile():
                    yield m, tar


def family(name):
    # non-incremental/QF_LIA/<family>/...
    parts = name.split("/")
    return parts[2] if len(parts) > 3 else ""


def main():
    archive, work = sys.argv[1], sys.argv[2]
    n = int(sys.argv[3]) if len(sys.argv) > 3 else 1000
    candidates = sorted(
        m.name for m, _ in members(archive)
        if m.name.endswith(".smt2") and family(m.name)
        and not any(f in family(m.name).lower() for f in EXCLUDED)
        and ".." not in m.name.split("/") and not m.name.startswith("/"))
    chosen = set(random.Random(SEED).sample(candidates,
                                            min(n, len(candidates))))
    print(f"{len(candidates)} candidate files, {len(chosen)} chosen")
    root = os.path.join(work, "QF_LIA-random")
    rows = []
    for m, tar in members(archive):
        if m.name not in chosen:
            continue
        dest = os.path.join(root, *m.name.split("/")[2:])
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "wb") as out:
            out.write(tar.extractfile(m).read())
        rows.append((family(m.name), dest))
    rows.sort()
    counts = {}
    for fam, _ in rows:
        counts[fam] = counts.get(fam, 0) + 1
    for fam, c in sorted(counts.items()):
        print(f"{c:5d} {fam}")
    with open(os.path.join(work, "files.tsv"), "w") as out:
        out.writelines(f"{fam}\t{path}\n" for fam, path in rows)
    print(f"{len(rows)} instances in {os.path.join(work, 'files.tsv')}")


if __name__ == "__main__":
    main()

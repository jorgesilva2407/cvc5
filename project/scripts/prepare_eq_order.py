"""Choose the instances of the equality-order experiment and make the
variants with implied equalities.

usage: prepare_eq_order.py QF_LIA_DIR WORKDIR

QF_LIA_DIR is the directory written by fetch_benchmarks.sh. Writes the
variants to WORKDIR/variants/ and the list of instances to
WORKDIR/files.tsv (lines "FAMILY<TAB>PATH"), which run_bench.py reads with
--file-list. The choice is fixed by SEED, so every configuration runs the
same instances.

Instances:
  - ORIGINAL[F] files of each family F, drawn at random (all if fewer);
  - for VARIANT_BASE of those files per family, the variants F+ie and F+ie2
    of make_implied_eq.py, which have implied equalities.
"""
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_implied_eq import VARIANTS, variant  # noqa: E402

SEED = 1
ORIGINAL = {
    "dillig": 50,
    "slacks": 50,
    "CAV_2009_benchmarks": 50,
    "20180326-Bromberger": 50,
    "prime-cone": 50,  # 37 files: all of them
}
VARIANT_BASE = 15


def smt2_files(root):
    return sorted(os.path.join(d, f) for d, _, fs in os.walk(root)
                  for f in fs if f.endswith(".smt2"))


def main():
    qflia, work = sys.argv[1], sys.argv[2]
    rows = []
    for family, n in ORIGINAL.items():
        root = os.path.join(qflia, family)
        files = smt2_files(root)
        if not files:
            sys.exit(f"no .smt2 files in {root}")
        chosen = sorted(random.Random(SEED).sample(files, min(n, len(files))))
        rows += [(family, f) for f in chosen]
        base = sorted(random.Random(SEED).sample(
            chosen, min(VARIANT_BASE, len(chosen))))
        for name, term in VARIANTS.items():
            vfamily = f"{family}+{name}"
            count = 0
            for f in base:
                with open(f) as fh:
                    text = variant(fh.read(), term)
                if text is None:
                    continue
                dest = os.path.join(work, "variants", vfamily,
                                    os.path.relpath(f, root))
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                with open(dest, "w") as out:
                    out.write(text)
                rows.append((vfamily, dest))
                count += 1
            print(f"{vfamily}: {count} files")
        print(f"{family}: {len(chosen)} files")
    with open(os.path.join(work, "files.tsv"), "w") as out:
        out.writelines(f"{fam}\t{path}\n" for fam, path in rows)
    print(f"{len(rows)} instances in {os.path.join(work, 'files.tsv')}")


if __name__ == "__main__":
    main()

"""Choose the instances of the report experiment: every file of the five
families of the proposal, and variants with implied equalities of them.

usage: prepare_report.py QF_LIA_DIR WORKDIR [--variants one|all|none]

QF_LIA_DIR is the directory written by fetch_benchmarks.sh. Writes the
variants to WORKDIR/report_variants/ and the list of instances to
WORKDIR/report_files.tsv (lines "FAMILY<TAB>PATH"), which run_bench.py reads
with --file-list. The choice is fixed by SEED, so every configuration, and
every later run, gets the same instances in the same order.

Instances:
  - all .smt2 files of each family F of FAMILIES (sets (i) and (ii));
  - the variants F+ie and F+ie2 of make_implied_eq.py (set (iii)): three
    constraints that imply b = c without stating it, so no unit cube fits,
    and the answer does not change. With --variants
      one   (default) one variant per file: the files of each family are
          shuffled (seed SEED) and alternately given the +ie and the +ie2
          variant, over all families together, so there are as many of each
          kind (950 and 950 for the 1,900 files), and each family is split
          in half (up to one file);
      all   both variants of every file;
      none  no variants.
"""
import argparse
import os
import random
import sys

# make_implied_eq.py is in the parent directory, with the shared scripts
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from make_implied_eq import VARIANTS, variant  # noqa: E402

FAMILIES = ["dillig", "slacks", "CAV_2009_benchmarks", "20180326-Bromberger",
            "prime-cone"]
SEED = 1


def smt2_files(root):
    return sorted(os.path.join(d, f) for d, _, fs in os.walk(root)
                  for f in fs if f.endswith(".smt2"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("qflia")
    ap.add_argument("work")
    ap.add_argument("--variants", choices=["one", "all", "none"],
                    default="one")
    args = ap.parse_args()
    kinds = list(VARIANTS)  # ["ie", "ie2"]
    rows, variant_rows = [], []
    nth = 0  # alternates the kind over all families together
    for family in FAMILIES:
        root = os.path.join(args.qflia, family)
        files = smt2_files(root)
        if not files:
            sys.exit(f"no .smt2 files in {root}")
        rows += [(family, f) for f in files]
        print(f"{family}: {len(files)} files")
        if args.variants == "none":
            continue
        if args.variants == "all":
            todo = [(f, k) for k in kinds for f in files]
        else:
            shuffled = files[:]
            random.Random(f"{SEED}-{family}").shuffle(shuffled)
            todo = []
            for f in shuffled:
                todo.append((f, kinds[nth % len(kinds)]))
                nth += 1
        count = {k: 0 for k in kinds}
        for f, k in sorted(todo, key=lambda x: (kinds.index(x[1]), x[0])):
            with open(f) as fh:
                text = variant(fh.read(), VARIANTS[k])
            if text is None:
                print(f"  no variant of {f} (no integer variable)")
                continue
            vfamily = f"{family}+{k}"
            dest = os.path.join(args.work, "report_variants", vfamily,
                                os.path.relpath(f, root))
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            with open(dest, "w") as out:
                out.write(text)
            variant_rows.append((vfamily, dest))
            count[k] += 1
        for k in kinds:
            print(f"{family}+{k}: {count[k]} files")
    rows += variant_rows
    path = os.path.join(args.work, "report_files.tsv")
    with open(path, "w") as out:
        out.writelines(f"{fam}\t{f}\n" for fam, f in rows)
    print(f"{len(rows)} instances in {path}")


if __name__ == "__main__":
    main()

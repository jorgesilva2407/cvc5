"""Make variants of benchmarks with implied equalities (proposal set (iii)).

usage: make_implied_eq.py BENCH_DIR OUTDIR [--sample N --seed S]
                          [--family NAME ...]

BENCH_DIR contains one directory per family (as written by
fetch_benchmarks.sh). For each family F, two variant families are written to
OUTDIR:

  F+ie   adds  x + a <= b,      b <= c,  c <= x + a
  F+ie2  adds  2x + 3a <= b,    b <= c,  c <= 2x + 3a

where x is the first integer variable the file declares and a, b, c are new
integer variables. The three constraints imply the equalities b = c = x + a
(resp. 2x + 3a) without containing any of them explicitly, so no unit cube
fits into any variant. Any solution of the original extends to one of the
variant (a = 0, b = c = x, resp. 2x), and the variant only adds constraints,
so the expected status in the file stays correct.

With --sample, the same N files per family are used as by run_bench.py with
the same --sample and --seed.
"""
import argparse
import os
import random
import re

DECL = re.compile(r"\((?:declare-fun\s+(\S+)\s+\(\s*\)|declare-const\s+(\S+))"
                  r"\s+Int\s*\)")
VARIANTS = {
    "ie": "(+ {x} {a})",
    "ie2": "(+ (* 2 {x}) (* 3 {a}))",
}


def select(root, sample, seed):
    # the same selection as select_files in run_bench.py
    found = sorted(os.path.join(d, f) for d, _, fs in os.walk(root)
                   for f in fs if f.endswith(".smt2"))
    if sample and len(found) > sample:
        found = sorted(random.Random(seed).sample(found, sample))
    return found


def variant(text, term):
    m = DECL.search(text)
    pos = text.find("(check-sat)")
    if not m or pos < 0:
        return None
    x = m.group(1) or m.group(2)
    names = {k: f"cube_ie_{k}" for k in "abc"}
    while any(n in text for n in names.values()):
        names = {k: "_" + n for k, n in names.items()}
    t = term.format(x=x, a=names["a"])
    add = "".join(f"(declare-fun {n} () Int)\n" for n in names.values())
    add += (f"(assert (<= {t} {names['b']}))\n"
            f"(assert (<= {names['b']} {names['c']}))\n"
            f"(assert (<= {names['c']} {t}))\n")
    return text[:pos] + add + text[pos:]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bench_dir")
    ap.add_argument("outdir")
    ap.add_argument("--sample", type=int, default=0)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--family", action="append", default=[])
    args = ap.parse_args()
    families = args.family or sorted(
        f for f in os.listdir(args.bench_dir)
        if os.path.isdir(os.path.join(args.bench_dir, f)) and "+" not in f)
    for family in families:
        root = os.path.join(args.bench_dir, family)
        files = select(root, args.sample, args.seed)
        for name, term in VARIANTS.items():
            written = skipped = 0
            for f in files:
                with open(f) as fh:
                    text = variant(fh.read(), term)
                if text is None:
                    skipped += 1
                    continue
                dest = os.path.join(args.outdir, f"{family}+{name}",
                                    os.path.relpath(f, root))
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                with open(dest, "w") as out:
                    out.write(text)
                written += 1
            print(f"{family}+{name}: {written} files"
                  + (f" ({skipped} without an Int variable or check-sat)"
                     if skipped else ""))


if __name__ == "__main__":
    main()

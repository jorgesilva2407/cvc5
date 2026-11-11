"""Summarize the results of run_bench.py.

usage: summarize.py RESULTS.csv [--timeout SEC] [--base B0]

Prints, for each family and configuration: solved instances, PAR-2, the time
on the instances solved by all configurations, wrong answers (against the
SMT-LIB status and against the base configuration), and the unit cube test
statistics. Then lists the instances whose outcome differs from the base.
"""
import argparse
import csv
import json
from collections import defaultdict

STATS = ["calls", "success", "skippedEquality", "skippedImpliedEquality",
         "noRoom", "shiftedInfeasible", "shiftedUnknown", "diseqFailed",
         "notApplicable", "impliedEq::calls", "impliedEq::rounds",
         "impliedEq::found", "impliedEq::unknown", "eqElim::calls",
         "eqElim::equalities", "eqElim::fresh", "eqElim::infeasible",
         "time", "impliedEq::time", "eqElim::time"]


def solved(r):
    return r["answer"] in ("sat", "unsat") and r["timeout"] != "True"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("results")
    ap.add_argument("--timeout", type=float, default=60)
    ap.add_argument("--base", default="B0")
    args = ap.parse_args()
    with open(args.results) as fh:
        rows = list(csv.DictReader(fh))
    configs = sorted(dict.fromkeys(r["config"] for r in rows),
                     key=lambda c: (c != args.base, c))
    by = {(r["file"], r["config"]): r for r in rows}
    files = defaultdict(set)
    for r in rows:
        files[r["family"]].add(r["file"])

    wrong = []
    for r in rows:
        if not solved(r):
            continue
        if r["expected"] in ("sat", "unsat") and r["answer"] != r["expected"]:
            wrong.append((r, "status " + r["expected"]))
        b = by.get((r["file"], args.base))
        if b and solved(b) and b["answer"] != r["answer"]:
            wrong.append((r, args.base + " says " + b["answer"]))

    for family in sorted(files) + ["ALL"]:
        fs = (set().union(*files.values()) if family == "ALL"
              else files[family])
        common = [f for f in fs
                  if all(solved(by[(f, c)]) for c in configs if (f, c) in by)]
        print(f"\n== {family}: {len(fs)} instances, "
              f"{len(common)} solved by all")
        print(f"{'config':16} {'solved':>6} {'sat':>4} {'unsat':>5} "
              f"{'t/o':>4} {'other':>5} {'PAR-2 s':>9} {'common s':>9} "
              f"{'wrong':>5}")
        for c in configs:
            rs = [by[(f, c)] for f in fs if (f, c) in by]
            nsolved = sum(solved(r) for r in rs)
            nsat = sum(solved(r) and r["answer"] == "sat" for r in rs)
            nto = sum(r["timeout"] == "True" for r in rs)
            par2 = sum(float(r["wall_s"]) if solved(r) else 2 * args.timeout
                       for r in rs)
            tcommon = sum(float(by[(f, c)]["wall_s"]) for f in common
                          if (f, c) in by)
            nwrong = sum(1 for r, _ in wrong
                         if r["config"] == c and r["file"] in fs)
            print(f"{c:16} {nsolved:6} {nsat:4} {nsolved - nsat:5} {nto:4} "
                  f"{len(rs) - nsolved - nto:5} {par2:9.1f} {tcommon:9.1f} "
                  f"{nwrong:5}")
        for c in configs:
            total = defaultdict(int)
            for f in fs:
                if (f, c) in by:
                    for k, v in json.loads(by[(f, c)]["stats"]).items():
                        total[k] += v
            if total:
                shown = ", ".join(
                    f"{k} {total[k]}" + ("ms" if k.endswith("time") else "")
                    for k in STATS if total.get(k))
                print(f"  {c}: {shown}")

    print(f"\n== wrong answers: {len(wrong)}")
    for r, why in wrong:
        print(f"  {r['config']} {r['file']}: {r['answer']} ({why})")
    print(f"\n== instances solved differently from {args.base}")
    for f in sorted(set().union(*files.values())):
        b = by.get((f, args.base))
        if not b:
            continue
        for c in configs:
            r = by.get((f, c))
            if r and c != args.base and solved(r) != solved(b):
                print(f"  {f}: {args.base} {b['answer']} "
                      f"{float(b['wall_s']):.1f}s, {c} {r['answer']} "
                      f"{float(r['wall_s']):.1f}s")


if __name__ == "__main__":
    main()

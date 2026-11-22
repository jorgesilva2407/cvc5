"""Report the propagation and overhead experiments from run_bench.py CSVs.

usage: report_experiments.py propagation RESULTS_DIR [--timeout 300]
                             [--control C1+C2] [--base B0]
       report_experiments.py overhead RESULTS_DIR [--timeout 60]
                             [--base B0] [--min-time 0.5]

RESULTS_DIR holds one CSV per configuration (as written by eq_order_run.sh
or overhead_run.sh); all.csv and *.files are ignored.

propagation: how much sending the implied equalities helps. For each
configuration: solved, PAR-2 and wrong answers, per family and in total, how
often the equalities were propagated, and the instances gained and lost
against the control (the same pipeline without propagation), with the time
ratio on the instances where something was propagated.

overhead: what the unit cube test costs. For each configuration other than
the base: the share of the run time spent in the test (the cube::time
statistic, only available for runs that finish), and the slowdown against
the base on the instances both solve where the test never succeeded, so it
was pure overhead (instances solved in under --min-time seconds by the base
are left out, since start-up dominates them).
"""
import argparse
import csv
import glob
import math
import os
import statistics
from collections import defaultdict


def load(directory):
    rows = defaultdict(dict)  # config -> file -> row
    for path in sorted(glob.glob(os.path.join(directory, "*.csv"))):
        if os.path.basename(path) == "all.csv":
            continue
        with open(path) as fh:
            for r in csv.DictReader(fh):
                rows[r["config"]][r["file"]] = r
    return rows


def num(r, col):
    v = r.get(col, "")
    return float(v) if v not in ("", None) else 0.0


def solved(r):
    return r["answer"] in ("sat", "unsat") and r["timeout"] != "True"


def wrong(r):
    return solved(r) and r["expected"] in ("sat", "unsat") \
        and r["answer"] != r["expected"]


def par2(r, timeout):
    return num(r, "wall_s") if solved(r) else 2 * timeout


def pct(values, q):
    if not values:
        return float("nan")
    values = sorted(values)
    return values[min(len(values) - 1, int(q * len(values)))]


def geomean(values):
    values = [v for v in values if v > 0]
    if not values:
        return float("nan")
    return math.exp(sum(math.log(v) for v in values) / len(values))


def order(configs, first):
    return [c for c in first if c in configs] + sorted(
        c for c in configs if c not in first)


def disagreements(rows):
    """Files where two configurations give different definite answers."""
    answers = defaultdict(set)
    for by_file in rows.values():
        for f, r in by_file.items():
            if solved(r):
                answers[f].add(r["answer"])
    return sorted(f for f, a in answers.items() if len(a) > 1)


def summary_table(rows, configs, timeout):
    common = set.intersection(*(set(rows[c]) for c in configs))
    all_solved = [f for f in common if all(solved(rows[c][f]) for c in configs)]
    print(f"{'config':16} {'solved':>7} {'wrong':>6} {'PAR-2 (s)':>11} "
          f"{'time on ' + str(len(all_solved)) + ' solved by all (s)':>30}")
    for c in configs:
        rs = [rows[c][f] for f in common]
        print(f"{c:16} {sum(map(solved, rs)):7d} {sum(map(wrong, rs)):6d} "
              f"{sum(par2(r, timeout) for r in rs):11.1f} "
              f"{sum(num(rows[c][f], 'wall_s') for f in all_solved):30.1f}")
    return common


def per_family(rows, configs, common, timeout):
    fams = sorted({rows[configs[0]][f]["family"] for f in common})
    print(f"\n{'family':28}" + "".join(f"{c:>22}" for c in configs))
    print(f"{'':28}" + "".join(f"{'solved  PAR-2':>22}" for _ in configs))
    for fam in fams:
        fs = [f for f in common if rows[configs[0]][f]["family"] == fam]
        cells = []
        for c in configs:
            rs = [rows[c][f] for f in fs]
            cells.append(f"{sum(map(solved, rs)):3d}/{len(rs):<3d}"
                         f"{sum(par2(r, timeout) for r in rs):9.0f}")
        print(f"{fam:28}" + "".join(f"{x:>22}" for x in cells))


def propagation(rows, args):
    configs = order(rows, [args.base, "C1", args.control])
    print(f"== Propagation of implied equalities ({len(configs)} configs, "
          f"timeout {args.timeout:g} s)\n")
    common = summary_table(rows, configs, args.timeout)
    per_family(rows, configs, common, args.timeout)
    bad = disagreements(rows)
    print(f"\nfiles with contradictory answers across configs: {len(bad)}")
    for f in bad:
        print(f"    {f}")
    if args.control not in rows:
        print(f"\n(no control configuration {args.control})")
        return
    ctrl = rows[args.control]
    for c in configs:
        if c in (args.base, args.control) or not any(
                num(r, "cube.impliedEq.propagated") for r in rows[c].values()):
            continue
        rs = rows[c]
        fs = sorted(set(rs) & set(ctrl))
        fired = [f for f in fs if num(rs[f], "cube.impliedEq.propagated")]
        lemmas = sum(num(rs[f], "cube.impliedEq.propagated") for f in fs)
        skip = sum(num(rs[f], "cube.impliedEq.onSkip") for f in fs)
        gained = [f for f in fs if solved(rs[f]) and not solved(ctrl[f])]
        lost = [f for f in fs if solved(ctrl[f]) and not solved(rs[f])]
        both = [f for f in fired if solved(rs[f]) and solved(ctrl[f])
                and num(ctrl[f], "wall_s") >= args.min_time]
        ratios = [num(rs[f], "wall_s") / num(ctrl[f], "wall_s") for f in both]
        print(f"\n-- {c} against {args.control}")
        print(f"  propagated on {len(fired)} of {len(fs)} instances, "
              f"{lemmas:.0f} lemmas"
              + (f" (C2 run on skip {skip:.0f} times)" if skip else "")
              + " (statistics of finished runs only)")
        by_fam = defaultdict(int)
        for f in fired:
            by_fam[rs[f]["family"]] += 1
        for fam, k in sorted(by_fam.items()):
            print(f"    {fam}: {k}")
        print(f"  solved only with propagation: {len(gained)}; "
              f"only without: {len(lost)}")
        for f in gained:
            print(f"    + {f} ({ctrl[f]['answer']}/{ctrl[f]['wall_s']}s -> "
                  f"{rs[f]['answer']}/{rs[f]['wall_s']}s)")
        for f in lost:
            print(f"    - {f} ({ctrl[f]['answer']}/{ctrl[f]['wall_s']}s -> "
                  f"{rs[f]['answer']}/{rs[f]['wall_s']}s)")
        if ratios:
            print(f"  time ratio {c}/{args.control} where propagated and both "
                  f"solved (>= {args.min_time:g} s): {len(ratios)} instances, "
                  f"geomean {geomean(ratios):.2f}, median "
                  f"{statistics.median(ratios):.2f}, "
                  f"min {min(ratios):.2f}, max {max(ratios):.2f}")


def overhead(rows, args):
    configs = order(rows, [args.base])
    print(f"== Overhead of the unit cube test ({len(configs)} configs, "
          f"timeout {args.timeout:g} s)\n")
    common = summary_table(rows, configs, args.timeout)
    bad = disagreements(rows)
    print(f"\nfiles with contradictory answers across configs: {len(bad)}")
    for f in bad:
        print(f"    {f}")
    base = rows[args.base]
    for c in configs:
        if c == args.base:
            continue
        rs = rows[c]
        fs = sorted(common)
        finished = [f for f in fs if rs[f]["answer"] != "none"
                    and rs[f]["timeout"] != "True"]
        called = [f for f in finished if num(rs[f], "cube.calls")]
        def share(f):
            return num(rs[f], "cube.time_ms") / 1000 / num(rs[f], "cpu_s")
        shares = {
            "test never succeeded": [share(f) for f in called
                                     if num(rs[f], "cpu_s") > 0
                                     and not num(rs[f], "cube.success")],
            "test succeeded": [share(f) for f in called
                               if num(rs[f], "cpu_s") > 0
                               and num(rs[f], "cube.success")]}
        test_s = sum(num(rs[f], "cube.time_ms") for f in finished) / 1000
        cpu_s = sum(num(rs[f], "cpu_s") for f in finished)
        helped = [f for f in finished if num(rs[f], "cube.success")]
        na = [f for f in finished if not num(rs[f], "cube.calls")
              and num(rs[f], "cube.notApplicable")]
        print(f"\n-- {c} against {args.base}")
        print(f"  finished runs: {len(finished)} of {len(fs)}; test called on "
              f"{len(called)}, succeeded at least once on {len(helped)}, "
              f"never applicable on {len(na)}")
        print(f"  time in the test: {test_s:.1f} s of {cpu_s:.1f} s CPU "
              f"({100 * test_s / cpu_s if cpu_s else 0:.2f}%)")
        for what, s in shares.items():
            if s:
                print(f"  share of CPU in the test, {what}: {len(s)} instances,"
                      f" median {100 * statistics.median(s):.2f}%, p90 "
                      f"{100 * pct(s, 0.9):.2f}%, max {100 * max(s):.2f}%")
        pure = [f for f in fs if solved(rs[f]) and solved(base[f])
                and not num(rs[f], "cube.success")
                and num(base[f], "cpu_s") >= args.min_time]
        ratios = [num(rs[f], "cpu_s") / num(base[f], "cpu_s") for f in pure]
        added = [num(rs[f], "cpu_s") - num(base[f], "cpu_s") for f in pure]
        print(f"  pure overhead (both solve, test never succeeded, base >= "
              f"{args.min_time:g} s): {len(pure)} instances")
        if ratios:
            print(f"    CPU ratio {c}/{args.base}: geomean {geomean(ratios):.3f}, "
                  f"median {statistics.median(ratios):.3f}, p90 "
                  f"{pct(ratios, 0.9):.3f}, max {max(ratios):.3f}")
            print(f"    added CPU time: total {sum(added):.1f} s, median "
                  f"{statistics.median(added):.2f} s")
        lost = [f for f in fs if solved(base[f]) and not solved(rs[f])]
        gained = [f for f in fs if solved(rs[f]) and not solved(base[f])]
        print(f"  solved only by {args.base}: {len(lost)}; only by {c}: "
              f"{len(gained)}")
        for f in lost:
            r = rs[f]
            print(f"    - {f} ({base[f]['wall_s']}s; {c}: {r['answer']}, "
                  f"{num(r, 'cube.calls'):.0f} calls)")
        by_fam = defaultdict(lambda: [0, 0.0, 0.0])
        for f in finished:
            e = by_fam[rs[f]["family"]]
            e[0] += 1
            e[1] += num(rs[f], "cube.time_ms") / 1000
            e[2] += num(rs[f], "cpu_s")
        print(f"  per family (finished runs): instances, time in test, "
              f"share of CPU")
        for fam, (k, t, cpu) in sorted(by_fam.items(),
                                       key=lambda kv: -kv[1][1])[:15]:
            print(f"    {fam:40} {k:5d} {t:9.2f} s "
                  f"{100 * t / cpu if cpu else 0:6.2f}%")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("experiment", choices=["propagation", "overhead"])
    ap.add_argument("results")
    ap.add_argument("--timeout", type=float)
    ap.add_argument("--base", default="B0")
    ap.add_argument("--control", default="C1+C2")
    ap.add_argument("--min-time", type=float, default=0.5)
    args = ap.parse_args()
    if args.timeout is None:
        args.timeout = 300 if args.experiment == "propagation" else 60
    rows = load(args.results)
    if args.base not in rows:
        raise SystemExit(f"no base configuration {args.base} in {args.results}")
    (propagation if args.experiment == "propagation" else overhead)(rows, args)


if __name__ == "__main__":
    main()

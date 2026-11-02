"""Run cvc5 configurations on benchmarks and record answers, times and the
unit cube test statistics.

usage: run_bench.py --binary CVC5 --bench-dir DIR --out RESULTS.csv
                    [--sample N --seed S] [--timeout SEC] [--jobs J]
                    [--mem-mb MB] [--config NAME=OPTIONS ...]

DIR contains one directory per family (as written by fetch_benchmarks.sh).
With --sample, N files of each family are drawn with seed S; the files used
are written next to the results (RESULTS.files). Rows already in the results
file are skipped, so an interrupted run can be resumed.
"""
import argparse
import csv
import json
import os
import random
import re
import resource
import subprocess
import sys
import tempfile
import threading
from concurrent.futures import ThreadPoolExecutor

DEFAULT_CONFIGS = {
    "B0": "",
    "B1": "--arith-cube-test",
    "B1+C2": "--arith-cube-test --arith-cube-implied-eq",
    "B2": "--arith-cube-test --arith-cube-implied-eq --arith-cube-eq-elim",
}
FIELDS = ["family", "file", "config", "answer", "expected", "returncode",
          "timeout", "wall_s", "cpu_s", "maxrss_kb", "stats"]
STAT = re.compile(r"^theory::arith::cube::(\S+) = (\d+)(ms)?$")
STATUS = re.compile(r"\(set-info :status (sat|unsat|unknown)\)")


def expected_status(path):
    with open(path) as fh:
        m = STATUS.search(fh.read())
    return m.group(1) if m else "unknown"


def select_files(bench_dir, sample, seed):
    files = []
    for family in sorted(os.listdir(bench_dir)):
        root = os.path.join(bench_dir, family)
        if not os.path.isdir(root):
            continue
        found = sorted(os.path.join(d, f) for d, _, fs in os.walk(root)
                       for f in fs if f.endswith(".smt2"))
        if sample and len(found) > sample:
            found = sorted(random.Random(seed).sample(found, sample))
        files += [(family, f) for f in found]
    return files


def run_one(binary, options, path, timeout, mem_mb):
    def limit():
        b = mem_mb * 1024 * 1024
        resource.setrlimit(resource.RLIMIT_AS, (b, b))

    with tempfile.NamedTemporaryFile("r", suffix=".time") as tf:
        cmd = ["/usr/bin/time", "-o", tf.name, "-f", "%e %U %S %M",
               "timeout", "--signal=KILL", str(timeout),
               binary, "--stats-internal", *options.split(), path]
        p = subprocess.run(cmd, capture_output=True, text=True,
                           preexec_fn=limit)
        timing = tf.read().strip().splitlines()
    wall, user, system, rss = timing[-1].split()
    answer = next((l.strip() for l in p.stdout.splitlines()
                   if l.strip() in ("sat", "unsat", "unknown")), "none")
    stats = {}
    for line in p.stderr.splitlines():
        m = STAT.match(line.strip())
        if m:
            stats[m.group(1)] = int(m.group(2))
    # /usr/bin/time reports the exit status of timeout, 137 if it killed cvc5
    rc = p.returncode
    return {"answer": answer, "returncode": rc,
            "timeout": rc in (124, 137) or float(wall) >= timeout,
            "wall_s": wall, "cpu_s": f"{float(user) + float(system):.2f}",
            "maxrss_kb": rss, "stats": json.dumps(stats, sort_keys=True)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", required=True)
    ap.add_argument("--bench-dir", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--sample", type=int, default=0)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--jobs", type=int, default=1)
    ap.add_argument("--mem-mb", type=int, default=1500)
    ap.add_argument("--config", action="append", default=[],
                    help="NAME=OPTIONS; default: B0, B1, B1+C2, B2")
    args = ap.parse_args()
    configs = dict(c.split("=", 1) for c in args.config) or DEFAULT_CONFIGS

    files = select_files(args.bench_dir, args.sample, args.seed)
    with open(args.out + ".files", "w") as fh:
        fh.writelines(f + "\n" for _, f in files)
    done = set()
    if os.path.exists(args.out):
        with open(args.out) as fh:
            done = {(r["file"], r["config"]) for r in csv.DictReader(fh)}
    todo = [(fam, f, name, opts) for fam, f in files
            for name, opts in configs.items() if (f, name) not in done]
    print(f"{len(files)} files, {len(configs)} configs, {len(todo)} runs to do",
          file=sys.stderr)

    new = not os.path.exists(args.out)
    out = open(args.out, "a", newline="")
    writer = csv.DictWriter(out, FIELDS)
    if new:
        writer.writeheader()
    lock = threading.Lock()
    count = [0]

    def task(item):
        fam, f, name, opts = item
        row = run_one(args.binary, opts, f, args.timeout, args.mem_mb)
        row.update(family=fam, file=f, config=name,
                   expected=expected_status(f))
        with lock:
            writer.writerow(row)
            out.flush()
            count[0] += 1
            if count[0] % 20 == 0:
                print(f"{count[0]}/{len(todo)}", file=sys.stderr)

    with ThreadPoolExecutor(args.jobs) as pool:
        list(pool.map(task, todo))
    out.close()


if __name__ == "__main__":
    main()

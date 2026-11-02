#!/bin/bash
# Mutation check: run in a throwaway git worktree with a configured debug
# build in WORKTREE/build, never in the main checkout.
# usage: mutate.sh WORKTREE OUTDIR   (ONLY=regex runs only matching mutants)
# For each mutant: apply one
# deliberate bug, rebuild, run the cube unit tests and regressions, restore.
WT=$(realpath "${1:?usage: mutate.sh WORKTREE OUTDIR}")
OUT=$(realpath -m "${2:?usage: mutate.sh WORKTREE OUTDIR}")
mkdir -p $OUT
cd $WT || exit 1

UC=src/theory/arith/linear/cube/unit_cube.cpp
ES=src/theory/arith/linear/cube/exact_simplex.cpp
TP=src/theory/arith/linear/theory_arith_private.cpp

# name|file|old line (fixed string)|new line
MUTANTS=(
  "M0-none|||"
  "M1-input-shift-quarter|$UC|    return Rational(1, 2);|    return Rational(1, 4);"
  "M2-row-shift-quarter|$UC|  return norm / Rational(2);|  return norm / Rational(4);"
  "M3-round-down|$UC|(z.getNoninfinitesimalPart() + half).floor()|(z.getNoninfinitesimalPart()).floor()"
  "M4-c2-no-strict-filter|$UC|      if (!strict[id])|      if (false)"
  "M5-skip-diseq-check|$TP|    if (DeltaRational(value[diseq->getVariable()]) == diseq->getValue())|    if (false \&\& DeltaRational(value[diseq->getVariable()]) == diseq->getValue())"
  "M6-short-explanation|$ES|    d_explanation.push_back(bound->d_id);|    ;"
)

for m in "${MUTANTS[@]}"; do
  IFS='|' read -r name file old new <<< "$m"
  if [ -n "$ONLY" ] && ! [[ "$name" =~ $ONLY ]]; then continue; fi
  log=$OUT/$name.log
  : > $log
  if [ -n "$file" ]; then
    cp $file $file.orig
    python3 -I - "$file" "$old" "$new" <<'EOF'
import sys
f, old, new = sys.argv[1], sys.argv[2], sys.argv[3].replace("\\&", "&")
s = open(f).read()
assert s.count(old) == 1, f"expected exactly one match, found {s.count(old)}"
open(f, "w").write(s.replace(old, new))
EOF
    if [ $? -ne 0 ] || cmp -s $file $file.orig; then
      echo "$name NOT-APPLIED" | tee -a $OUT/summary.txt
      mv $file.orig $file; touch $file
      continue
    fi
    diff $file.orig $file >> $log
  fi
  (cd build && make -j2 cvc5-bin arith_unit_cube_white arith_exact_simplex_white arith_equality_elimination_white) >> $log 2>&1
  if [ $? -ne 0 ]; then
    echo "$name BUILD-FAILED" | tee -a $OUT/summary.txt
  else
    res="$name"
    for t in arith_unit_cube_white arith_exact_simplex_white arith_equality_elimination_white; do
      o=$(timeout 900 build/bin/test/unit/theory/$t 2>&1); rc=$?
      echo "$o" > $OUT/$name.$t.out
      failed=$(echo "$o" | grep -c '^\[  FAILED  \] Test')
      crash=$(echo "$o" | grep -m1 -iE 'assertion|check failure|fatal' | cut -c1-100)
      res="$res | $t rc=$rc failed=$failed ${crash:+crash=\"$crash\"}"
    done
    o=$(cd build && timeout 1800 ctest -R cube-test 2>&1); echo "$o" > $OUT/$name.ctest.out
    res="$res | regress: $(echo "$o" | grep -E 'tests passed')"
    echo "$res" | tee -a $OUT/summary.txt
  fi
  if [ -n "$file" ]; then
    # touch, so that make rebuilds the restored file
    mv $file.orig $file; touch $file
  fi
done
echo ALL-DONE

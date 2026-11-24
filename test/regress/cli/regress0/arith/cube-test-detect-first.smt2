; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(calls|success|skippedImpliedEquality|noRoom|shiftedInfeasible|impliedEq::calls|impliedEq::found) '
; EXPECT-ERROR: theory::arith::cube::calls = 1
; EXPECT-ERROR: theory::arith::cube::impliedEq::calls = 1
; EXPECT-ERROR: theory::arith::cube::impliedEq::found = 3
; EXPECT-ERROR: theory::arith::cube::skippedImpliedEquality = 1
; x <= y <= z <= x implies x = y = z (see cube-test-implied-cycle.smt2). With
; detect-first, the implied equalities are found before the test, which is
; then skipped: the shifted system is never solved.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (>= (+ (* 2 x) (* 3 y)) 1))
(check-sat)

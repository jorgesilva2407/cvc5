; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-propagate-eq --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(impliedEq::found|impliedEq::propagated|skippedEquality) '
; EXPECT-ERROR: theory::arith::cube::impliedEq::found = 3
; EXPECT-ERROR: theory::arith::cube::impliedEq::propagated = 3
; EXPECT-ERROR: theory::arith::cube::skippedEquality = 1
; x <= y <= z <= x implies x = y = z (see cube-test-implied-cycle.smt2). The
; implied equalities are sent as lemmas, after which they are explicit and
; the test is skipped.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (>= (+ (* 2 x) (* 3 y)) 1))
(check-sat)

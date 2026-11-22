; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first --arith-cube-propagate-eq --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(impliedEq::found|impliedEq::propagated|skippedImpliedEquality) '
; EXPECT-ERROR: theory::arith::cube::impliedEq::found = 3
; EXPECT-ERROR: theory::arith::cube::impliedEq::propagated = 3
; EXPECT-ERROR: theory::arith::cube::skippedImpliedEquality = 1
; x + y <= 2, x >= 1 and y >= 1 imply x = 1, y = 1 and x + y = 2 (see
; cube-test-implied-bounds.smt2). They are found before the test, which is
; skipped, and sent as lemmas.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= (+ x y) 2))
(assert (>= x 1))
(assert (>= y 1))
(assert (>= (- (* 2 z) x) 2))
(check-sat)

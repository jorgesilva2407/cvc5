; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first --arith-cube-eq-elim --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(success|skippedEquality|skippedImpliedEquality|impliedEq::calls|impliedEq::found|eqElim::calls|eqElim::equalities) '
; EXPECT-ERROR: theory::arith::cube::eqElim::calls = 1
; EXPECT-ERROR: theory::arith::cube::eqElim::equalities = 1
; EXPECT-ERROR: theory::arith::cube::impliedEq::calls = 1
; EXPECT-ERROR: theory::arith::cube::success = 1
; Only an explicit equality, x + y = 3 (see cube-test-eqelim.smt2). With
; detect-first and equality elimination, the detection runs first and finds
; no implied equality (explicit ones are not reported), so the test is not
; skipped: the explicit equality is eliminated, leaving 5x >= 11, and the test
; succeeds on it.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (>= (+ x y) 3))
(assert (<= (+ x y) 3))
(assert (>= (- (* 2 x) (* 3 y)) 2))
(check-sat)

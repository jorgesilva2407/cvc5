; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-eq-elim --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(success|shiftedInfeasible|impliedEq::found|eqElim::calls|eqElim::equalities) '
; EXPECT-ERROR: theory::arith::cube::eqElim::calls = 1
; EXPECT-ERROR: theory::arith::cube::eqElim::equalities = 3
; EXPECT-ERROR: theory::arith::cube::impliedEq::found = 3
; EXPECT-ERROR: theory::arith::cube::shiftedInfeasible = 1
; EXPECT-ERROR: theory::arith::cube::success = 1
; x <= y <= z <= x implies x = y = z (see cube-test-implied-cycle.smt2). The
; implied equalities are detected and eliminated, which leaves 5x >= 1.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (>= (+ (* 2 x) (* 3 y)) 1))
(check-sat)

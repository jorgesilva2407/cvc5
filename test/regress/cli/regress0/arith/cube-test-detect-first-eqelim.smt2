; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first --arith-cube-eq-elim --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(calls|success|skippedImpliedEquality|shiftedInfeasible|impliedEq::calls|impliedEq::found|eqElim::calls|eqElim::equalities) '
; EXPECT-ERROR: theory::arith::cube::calls = 1
; EXPECT-ERROR: theory::arith::cube::eqElim::calls = 1
; EXPECT-ERROR: theory::arith::cube::eqElim::equalities = 3
; EXPECT-ERROR: theory::arith::cube::impliedEq::calls = 1
; EXPECT-ERROR: theory::arith::cube::impliedEq::found = 3
; EXPECT-ERROR: theory::arith::cube::success = 1
; x <= y <= z <= x implies x = y = z (see cube-test-implied-cycle.smt2). With
; detect-first and equality elimination, the paper's order (Sec. 6): the
; implied equalities are found before the test, eliminated, and the test is
; run once, on the reduced problem 5x >= 1, where it succeeds. Unlike
; test-first (cube-test-eqelim-implied.smt2), the shifted system of the
; original problem is never solved, so it is never found infeasible.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (>= (+ (* 2 x) (* 3 y)) 1))
(check-sat)

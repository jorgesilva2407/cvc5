; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-eq-elim --arith-cube-propagate-eq --stats-internal
; EXPECT: unsat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(impliedEq::calls|impliedEq::propagated) '
; EXPECT-ERROR: theory::arith::cube::impliedEq::calls = 2
; EXPECT-ERROR: theory::arith::cube::impliedEq::propagated = 3
; x <= y <= z <= x implies x = y = z, and then 3x + 5y = 4 becomes 8x = 4,
; which has no integer solution (3x + 5y = 4 alone has one). The implied
; equalities are first found on the problem reduced by 3x + 5y = 4, whose
; variables are not those of the solver, so they are found again on the
; original problem to be propagated.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (= (+ (* 3 x) (* 5 y)) 4))
(check-sat)

; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-eq-elim --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(success|eqElim::calls|eqElim::equalities|eqElim::fresh) '
; EXPECT-ERROR: theory::arith::cube::eqElim::calls = 1
; EXPECT-ERROR: theory::arith::cube::eqElim::equalities = 1
; EXPECT-ERROR: theory::arith::cube::eqElim::fresh = 2
; EXPECT-ERROR: theory::arith::cube::success = 1
; 3x + 5y = 7 has no coefficient of 1, so its elimination needs a fresh
; variable. Its integer solutions are x = 4 + 5t, y = -1 - 3t.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (= (+ (* 3 x) (* 5 y)) 7))
(assert (>= x 1))
(check-sat)

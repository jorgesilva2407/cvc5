; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-eq-elim --stats-internal
; EXPECT: unsat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(success|shiftedInfeasible|eqElim::calls) '
; EXPECT-ERROR: theory::arith::cube::eqElim::calls = 1
; EXPECT-ERROR: theory::arith::cube::shiftedInfeasible = 1
; The real relaxation is satisfiable (x = 1/3, y = 0), but the integer
; solutions x = 2 + 5t, y = -1 - 3t of 3x + 5y = 1 are never both
; non-negative. The elimination must not interfere.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (= (+ (* 3 x) (* 5 y)) 1))
(assert (>= x 0))
(assert (>= y 0))
(check-sat)

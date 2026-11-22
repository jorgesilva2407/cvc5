; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-propagate-eq
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-eq-elim --arith-cube-propagate-eq
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first --arith-cube-propagate-eq
; EXPECT: unsat
; x <= y <= z <= x, x + 2y >= 1 and 2x + y <= 1 have the single rational
; solution x = y = z = 1/3, so every bound is an implied equality, and there
; is no integer solution. The propagated equalities must be valid: the
; answer is unsat with and without them.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (>= (+ x (* 2 y)) 1))
(assert (<= (+ (* 2 x) y) 1))
(check-sat)

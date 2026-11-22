; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-propagate-eq --arith-cube-propagate-eq-on-skip
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first --arith-cube-propagate-eq --arith-cube-propagate-eq-on-skip
; EXPECT: unsat
; x <= y <= z <= x implies x = y = z, and then the explicit equality
; 3x + 5y = 4 becomes 8x = 4, which has no integer solution. The test is
; skipped because of the explicit equality; the implied equalities are
; detected and sent anyway.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (= (+ (* 3 x) (* 5 y)) 4))
(check-sat)

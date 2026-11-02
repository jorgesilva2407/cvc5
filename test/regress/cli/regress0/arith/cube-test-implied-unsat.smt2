; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq
; EXPECT: unsat
; The implied equality 3x + 5y = 1 holds over the reals, but has no
; non-negative integer solution. The implied equality detection must not
; produce a conflict.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (>= x 0))
(assert (>= y 0))
(assert (>= (+ (* 3 x) (* 5 y)) 1))
(assert (<= (+ (* 3 x) (* 5 y)) 1))
(check-sat)

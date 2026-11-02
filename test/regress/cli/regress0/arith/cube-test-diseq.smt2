; COMMAND-LINE: --arith-cube-test --check-models
; EXPECT: sat
; Disequalities must also hold at the rounded point.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (>= (- (* 3 x) (* 7 y)) 1))
(assert (>= (+ (* 2 x) (* 5 y)) 3))
(assert (distinct x 1 2 3))
(assert (distinct y 0 1))
(assert (distinct (+ x y) 2 3 4))
(check-sat)

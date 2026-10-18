; COMMAND-LINE: --arith-cube-test --check-models
; EXPECT: sat
; The implied equality x + y = 3 leaves no room for a unit cube.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (>= (+ x y) 3))
(assert (<= (+ x y) 3))
(assert (>= (- (* 2 x) (* 3 y)) 1))
(check-sat)

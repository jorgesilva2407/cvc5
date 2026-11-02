; COMMAND-LINE: --arith-cube-test --check-models
; EXPECT: sat
; Boolean structure: the test runs at several full effort checks.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(declare-fun p () Bool)
(assert (or (and (>= (- (* 3 x) (* 7 y)) 1) (<= (+ x y) (- 50)))
            (and (>= (+ (* 2 x) (* 5 y)) 3) (>= (- (* 4 y) (* 9 z)) 2))))
(assert (or p (>= (+ (* 6 x) (* 11 z)) 7)))
(assert (or (not p) (<= (- (* 13 y) (* 2 z)) (- 5))))
(assert (or (= x (* 2 z)) (>= (+ x z) 17)))
(check-sat)

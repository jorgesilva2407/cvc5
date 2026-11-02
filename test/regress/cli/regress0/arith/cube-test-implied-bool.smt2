; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --check-models
; EXPECT: sat
; Implied equalities under Boolean structure: they change between full
; effort checks.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(declare-fun p () Bool)
(assert (or p (and (<= x y) (<= y x))))
(assert (or (not p) (and (<= (+ x z) 4) (>= x 2) (>= z 2))))
(assert (>= (- (* 3 x) (* 7 y)) 1))
(assert (distinct z 3))
(check-sat)

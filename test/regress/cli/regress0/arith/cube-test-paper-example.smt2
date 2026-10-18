; COMMAND-LINE: --arith-cube-test --check-models
; EXPECT: sat
; Example from Bromberger and Weidenbach (IJCAR 2016): the only integer
; solution is (1, 3), no unit cube fits, and branching must find it.
(set-logic QF_LIA)
(declare-fun x1 () Int)
(declare-fun x2 () Int)
(assert (<= (- (* 3 x1) x2) 0))
(assert (<= (- (* (- 2) x1) x2) (- 2)))
(assert (<= (+ (* (- 2) x1) x2) 1))
(check-sat)

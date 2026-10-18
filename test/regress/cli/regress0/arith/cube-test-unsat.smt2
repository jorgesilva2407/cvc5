; COMMAND-LINE: --arith-cube-test
; EXPECT: unsat
; The real relaxation is satisfiable, but 3x + 5y is never 1 or 2 for
; non-negative x and y. The cube test must not interfere.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (>= x 0))
(assert (>= y 0))
(assert (>= (+ (* 3 x) (* 5 y)) 1))
(assert (<= (+ (* 3 x) (* 5 y)) 2))
(check-sat)

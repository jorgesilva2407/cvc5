; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-propagate-eq --arith-cube-propagate-eq-form=equality
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-propagate-eq --arith-cube-propagate-eq-form=equality --no-arith-rewrite-equalities
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-eq-elim --arith-cube-propagate-eq --arith-cube-propagate-eq-form=equality --no-arith-rewrite-equalities
; EXPECT: unsat
; As cube-test-propagate-unsat.smt2, with the implied equalities sent as
; equalities instead of bounds; with --no-arith-rewrite-equalities they stay
; equality atoms.
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

; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-propagate-eq --arith-cube-propagate-eq-form=equality --no-arith-rewrite-equalities --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::impliedEq::propagated '
; EXPECT-ERROR: theory::arith::cube::impliedEq::propagated = 3
; As cube-test-propagate-cycle.smt2, with the implied equalities sent as
; equality atoms.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (>= (+ (* 2 x) (* 3 y)) 1))
(check-sat)

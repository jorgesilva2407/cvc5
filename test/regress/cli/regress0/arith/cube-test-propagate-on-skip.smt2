; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-propagate-eq --arith-cube-propagate-eq-on-skip --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(impliedEq::onSkip|impliedEq::propagated) '
; EXPECT-ERROR: theory::arith::cube::impliedEq::onSkip = 3
; EXPECT-ERROR: theory::arith::cube::impliedEq::propagated = 3
; The explicit equality 3w + 5x = 7 makes the test skip before the implied
; equalities x = y = z (from x <= y <= z <= x) are detected. With
; --arith-cube-propagate-eq-on-skip they are detected and sent anyway.
(set-logic QF_LIA)
(declare-fun w () Int)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (= (+ (* 3 w) (* 5 x)) 7))
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (>= (+ (* 2 x) (* 3 y)) 1))
(check-sat)

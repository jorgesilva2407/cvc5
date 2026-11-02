; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(skippedEquality|noRoom|success) '
; EXPECT-ERROR: theory::arith::cube::noRoom = 2
; There is no equality, but 1 <= 2x - 3y <= 4 is narrower than the shift
; 1/2 (|2| + |-3|) on each side, so no unit cube fits. The test is called
; twice, before and after a branch.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (>= (- (* 2 x) (* 3 y)) 1))
(assert (<= (- (* 2 x) (* 3 y)) 4))
(assert (>= (+ (* 3 x) (* 5 y)) 7))
(check-sat)

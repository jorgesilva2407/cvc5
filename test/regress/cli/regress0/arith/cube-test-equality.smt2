; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(skippedEquality|noRoom|success) '
; EXPECT-ERROR: theory::arith::cube::skippedEquality = 1
; The equality x + y = 3 leaves no room for a unit cube. The real relaxation
; is not integral (x >= 11/5), so the test is called.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (>= (+ x y) 3))
(assert (<= (+ x y) 3))
(assert (>= (- (* 2 x) (* 3 y)) 2))
(check-sat)

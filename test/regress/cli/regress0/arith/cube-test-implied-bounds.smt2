; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(impliedEq::found|impliedEq::rounds) '
; EXPECT-ERROR: theory::arith::cube::impliedEq::found = 3
; EXPECT-ERROR: theory::arith::cube::impliedEq::rounds = 2
; x + y <= 2, x >= 1 and y >= 1 imply x = 1, y = 1 and x + y = 2.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= (+ x y) 2))
(assert (>= x 1))
(assert (>= y 1))
(assert (>= (- (* 2 z) x) 2))
(check-sat)

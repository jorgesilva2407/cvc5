; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(success) '
; EXPECT-ERROR: theory::arith::cube::success = 1
; The polyhedron has infinite lattice width, so the unit cube test succeeds.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (>= (- (* 3 x) (* 7 y)) 1))
(assert (>= (+ (* 2 x) (* 5 y)) 3))
(assert (>= (+ (* 4 x) (* (- 9) y) (* 11 z)) 5))
(assert (<= (+ (* 5 y) (* (- 3) z)) 2))
(check-sat)

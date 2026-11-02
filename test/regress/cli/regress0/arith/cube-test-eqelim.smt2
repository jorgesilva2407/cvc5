; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-eq-elim --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(skippedEquality|noRoom|success|eqElim::calls|eqElim::equalities|eqElim::fresh) '
; EXPECT-ERROR: theory::arith::cube::eqElim::calls = 1
; EXPECT-ERROR: theory::arith::cube::eqElim::equalities = 1
; EXPECT-ERROR: theory::arith::cube::success = 1
; The equality x + y = 3 leaves no room for a unit cube (see
; cube-test-equality.smt2). Eliminating it leaves 5x >= 11 (with y = 3 - x),
; which has room.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(assert (>= (+ x y) 3))
(assert (<= (+ x y) 3))
(assert (>= (- (* 2 x) (* 3 y)) 2))
(check-sat)

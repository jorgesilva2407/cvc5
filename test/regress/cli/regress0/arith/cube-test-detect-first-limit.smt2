; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --arith-cube-implied-eq --arith-cube-implied-eq-order=detect-first --arith-cube-implied-eq-pivot-limit=1 --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(skippedImpliedEquality|shiftedInfeasible|impliedEq::found|impliedEq::unknown) '
; EXPECT-ERROR: theory::arith::cube::impliedEq::unknown = 1
; EXPECT-ERROR: theory::arith::cube::shiftedInfeasible = 1
; With at most one pivot per call, the detection gives up before it finds the
; implied equalities x = y = z; then the test is run anyway and fails, and
; branching finds the model.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (<= x y))
(assert (<= y z))
(assert (<= z x))
(assert (>= (+ (* 2 x) (* 3 y)) 1))
(check-sat)

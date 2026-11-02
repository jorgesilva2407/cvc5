; REQUIRES: statistics
; COMMAND-LINE: --arith-cube-test --no-arith-rewrite-equalities --check-models --stats-internal
; EXPECT: sat
; ERROR-SCRUBBER: grep -E '^theory::arith::cube::(diseqFailed|success) '
; EXPECT-ERROR: theory::arith::cube::diseqFailed = 1
; The shifted bounds of x are [1/2, 19/2] and the test rounds x = 1/2 to 1,
; which violates x != 1, so the point is rejected and branching continues.
; In pure linear integer logics, equalities are rewritten into inequalities by
; default, so disequalities only reach the arithmetic solver with
; --no-arith-rewrite-equalities.
(set-logic QF_LIA)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (>= x 0))
(assert (<= x 10))
(assert (distinct x 1))
(assert (>= (- (* 2 y) (* 3 z)) 1))
(assert (>= (+ (* 5 y) z) 4))
(check-sat)

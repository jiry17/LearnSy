(set-logic LIA)

(synth-fun combine ((lans Int) (rans Int) (lhead Int) (rhead Int) (ltail Int) (rtail Int)) Int
    ((Start Int (lans
                 rans
                 lhead
                 rhead
                 ltail
                 rtail
                 0
                 1
                 (+ Start Start)
                 (- Start Start)
                 (ite StartBool Start Start)))
     (StartBool Bool ((and StartBool StartBool)
                      (or  StartBool StartBool)
                      (not StartBool)
                      (<  Start Start)
                      (=   Start Start)
                      (<=  Start Start)))))


(define-fun ans ((lans Int) (rans Int) (lhead Int) (rhead Int) (ltail Int) (rtail Int)) Int
  (ite (and (= 1 ltail) (= 0 rhead))
       (- (+ lans rans) 1)
       (+ lans rans)))


(define-fun ib ((x Int)) Bool (and (>= x 0) (<= x 1)))

(declare-var a Int)
(declare-var b Int)
(declare-var c Int)
(declare-var d Int)
(declare-var e Int)
(declare-var f Int)
(constraint (=> (and (and (and (ib a) (ib b)) (and (ib c) (ib d))) (and (ib e) (ib f))) (= (ans a b c d e f) (combine a b c d e f))))

(check-synth)


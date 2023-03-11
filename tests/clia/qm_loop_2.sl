(set-logic LIA)

(define-fun qm ((a Int) (b Int)) Int
      (ite (< a 0) b a))

(synth-fun qm-multi-loop ((x Int) (y Int) ) Int
    ((Start Int (x y
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


(declare-var x Int)
(declare-var y Int)

(constraint (or  (or (< x 0) (< y 0)) 
                 (= (qm-multi-loop x y)  
                      (ite (= y 0) (ite (= x 0) 3 (- x 1)) x) ) ))

(check-synth)

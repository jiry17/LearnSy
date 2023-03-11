(set-logic LIA)

(define-fun qm ((a Int) (b Int)) Int
      (ite (< a 0) b a))

(synth-fun qm-foo ((x Int) (y Int) ) Int
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

(constraint (= (qm-foo x y)  
                (ite (and (< x 0)  (< y 0) ) 1 0))) 

(check-synth)

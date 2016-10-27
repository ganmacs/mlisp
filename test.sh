#!/bin/bash

fail() {
    echo -e -n "\033[0;31m[ERROR] \033[0;39m"
    echo "$1"
    exit 1
}

do_run() {
    echo "$2" | ./mlisp 2> /dev/null
    if [ "$?" -ne 0 ]; then
        echo FAILED
        fail "$error"
    fi

    result=$(echo "$2" | ./mlisp 2> /dev/null)
    if [ "$result" != "$3" ]; then
        echo FAILED
        fail "$3 expected, but got $result"
    fi
}

run() {
    echo -n "- Testing $1 ... "
    do_run "$@"
}

parse_run() {
    MLISP_PARSE_TEST=1 run "$@"
}

eval_run() {
    MLISP_EVAL_TEST=1 run "$@"
}

echo -e "\n== Parse test =="

parse_run int "1" "1"
parse_run symbol "x" "x"
parse_run true "t" "t"
parse_run nil "()" "nil"
parse_run list "(+ 1 2 10 1000 100000)" "(+ 1 2 10 1000 100000)"
parse_run list2 "(+ a bc cde abc10)" "(+ a bc cde abc10)"
parse_run "less than" "(< 10 11)" "(< 10 11)"
parse_run dot "(10 . 20)" "(10 . 20)"
parse_run fun "(inc 10)" "(inc 10)"
parse_run "inner cell" "(- (+ 10 11) (+ 12 13))" "(- (+ 10 11) (+ 12 13))"
parse_run quote "'(1 2 3)" "(quote (1 2 3))"
parse_run quote "'1" "(quote 1)"
parse_run quote_nil "'(1 () 3)" "(quote (1 nil 3))"
parse_run nil "'()" "(quote nil)"
parse_run lambda "((lambda (x) (+ x 1)) 10)" "((lambda (x) (+ x 1)) 10)"
parse_run let "(let ((x 10)) (+ x 1))" "(let ((x 10)) (+ x 1))"
parse_run if "(if () (+ 1 2) (+ 2 3))" "(if nil (+ 1 2) (+ 2 3))"

echo -e "\n== Eval test =="

eval_run int "3" 3
eval_run plus "(+ 1 2)" 3
eval_run add_3_args "(+ 1 2 10)" 13
eval_run minus "(- 2 1)" 1
eval_run minus_3_args "(- 10 1 2)" 7
eval_run mul "(* 2 1)" 2
eval_run mul_3_args "(* 10 1 2)" 20
eval_run div "(/ 2 1)" 2
eval_run div_3_args "(/ 10 1 2)" 5

eval_run "equal int" "(= 10 10 10)" "t"
eval_run "equal int" "(= 10 11 10)" "()"
eval_run "less than" "(< 10 11)" "t"
eval_run "less than" "(< 10 8)" "()"
eval_run "less than" "(< 10 11 12)" "t"
eval_run "less than" "(<= 10 10 12)" "t"
eval_run "greater than" "(> 10 8)" "t"
eval_run "greater than" "(> 10 11)" "()"
eval_run "greater than" "(> 12 11 10)" "t"
eval_run "greater than" "(>= 12 11 1)" "t"

eval_run car "(car '(1 2 3))" 1
eval_run car "(car '((1) 2 3))" "(1)"
eval_run car "(car '((+ 2 1) 2 3))" "(+ 2 1)"
eval_run cdr "(cdr '(1 2 3))" "(2 3)"
eval_run cdr "(cdr '((1) 2 3))" "(2 3)"
eval_run cdr "(cdr '((1) (+ 1 2) 3))" "((+ 1 2) 3)"
eval_run cdr "(cdr '((1) (+ 1 2) 3 10 20 30))" "((+ 1 2) 3 10 20 30)"

eval_run cons "(cons 1 '())" "(1)"
eval_run cons "(cons (+ 1 2) '())" "(3)"
eval_run cons2 "(cons 1 '(2 3))" "(1 2 3)"
eval_run cons2 "(cons '(1 2) '(3 4))" "((1 2) 3 4)"

eval_run quote "'(1 2 3)" "(1 2 3)"
eval_run quote "'((2 3) 2 3)" "((2 3) 2 3)"
eval_run quote "'(1 (+ 2 3) 4)" "(1 (+ 2 3) 4)"

eval_run progn '(progn 1)' 1
eval_run progn2 '(progn (+ 1 2) 10)' 10
eval_run progn2 '(progn 10 (+ 1 2))' 3
eval_run progn3 '(progn 10 (* 1 2) (* 10 30))' 300
eval_run progn3 '(progn 10 (* 1 2) (progn 1 (+ 10 20)))' 30

eval_run let '(let ((x 10)) (+ 10 x))' 20
eval_run let2 '(let ((x 10) (y 20)) (+ y 10 x))' 40
eval_run let2 '(let ((x 10) (y 20)) (progn (+ y 10 x) (* y x)))' 200

eval_run if "(if 1 1)" 1
eval_run if "(if () 1 3)" 3
eval_run if "(if () 1)" "()"
eval_run if "(if () (+ 1 2) (+ 2 3))" 5
eval_run if "(if t (progn 20 (+ (+ 1 2) (+ 1 2))) (+ 2 3))" 6
eval_run if "(if () (+ 1 2) (+ 2 3) (+ 2 3))" 5

eval_run define '(define x 7)' 7
eval_run define '(progn (define x 7) x)' 7
eval_run varible '(progn (define x 7) (+ x 10))' 17
eval_run varible '(progn (define x 7) (define y 10) (+ x y))' 17

eval_run lambda '((lambda (x) (+ x 1)) 10)' 11
eval_run lambda2 '((lambda (x y) (+ x 1 y)) 10 20)' 31
eval_run lambda_with_lambda '((lambda (f1 f2) (f2 (f1 10) (f1 20))) (lambda (x) x) (lambda (x y) (* x y)))' 200

eval_run defun '(progn (defun fn (x y) (+ x y)) (fn 10 20))' 30
eval_run defun '(progn (defun fn (x y) (+ x y)) (defun fn2 (x y) (+ x y)) (fn (fn 1 2) (fn2 1 2)))' 6

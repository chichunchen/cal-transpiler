# calculator language transpiler

### Feature

- Parse extended caculator with top-down descent approach
- Syntax Error Detection/Recovery
    - Exception based
    - Context specific look ahead for immediate error detection
- Construct AST
- Static semantic check for do/check
    - Every check statement appears inside a do statement
    - Every do statement has at least one check statement that is inside it and not inside any nested do. 
- Translate to C

### Extended Grammar

- Here the new nonterminal R is meant to suggest a “relation.”
As in C, a value of 0 is taken to be false; anything else is true.
- The relational operators (==, <> [not equal], <, >, <=, and >=) produce either 0 or 1 when evaluated.
- A do loop is intended to iterate until some check-ed relation inside it evaluates to false— “check R” is analogous to “if (!R) break” in C.

```
P   →   SL $$
SL  →   S SL  |  ε
S   →   id := R  |  read id  |  write R  |  if R SL fi  |  do SL od  |  check R
R   →   E ET
E   →   T TT
T   →   F FT
F   →   ( R )  |  id  |  lit
ET  →   ro E  |  ε
TT  →   ao T TT  |  ε
FT  →   mo F FT  |  ε
ro  →   ==  |  <>  |  <  |  >  |  <=  |  >=
ao  →   +  |  -
mo  →   *  |  /
```

### Sample Input
```
read n
cp := 2
do check n > 0
   found := 0
   cf1 := 2
   cf1s := cf1 * cf1
   do check cf1s <= cp
       cf2 := 2
       pr := cf1 * cf2
       do check pr <= cp
           if pr == cp
               found := 1
           fi
           cf2 := cf2 + 1
           pr := cf1 * cf2
       od
       cf1 := cf1 + 1
       cf1s := cf1 * cf1
   od
   if found == 0
       write cp
       n := n - 1
   fi
   cp := cp + 1
od
$$  <--- do not need to write in the file I guess
```

### AST Output
```
(program
  [ (read "n")
    (:= "cp" (num "2"))
    (do
      [ (check > (id "n") (num "0"))
        (:= "found" (num "0"))
        (:= "cf1" (num "2"))
        (:= "cf1s" (* (id "cf1") (id "cf1")))
        (do
          [ (check <= (id "cf1s") (id "cp"))
            (:= "cf2" (num "2"))
            (:= "pr" (* (id "cf1") (id "cf2")))
            (do
              [ (check <= (id "pr") (id "cp"))
                (if
                  (== (id "pr") (id "cp"))
                  [ (:= "found" (num "1"))
                  ]
                )
                (:= "cf2" (+ (id "cf2") (num "1")))
                (:= "pr" (* (id "cf1") (id "cf2")))
              ]
            )
            (:= "cf1" (+ (id "cf1") (num "1")))
            (:= "cf1s" (* (id "cf1") (id "cf1")))
          ]
        )
        (if
          (== (id "found") (num "0"))
          [ (write (id "cp"))
            (:= "n" (- (id "n") (num "1")))
          ]
        )
        (:= "cp" (+ (id "cp") (num "1")))
      ]
    )
  ]
)
```


### Tests
Makefile test the output of ast and static semantic check.
```
./parse < test01.txt > output01.txt               
diff --ignore-all-space correct01.txt output01.txt
./parse < test02.txt > output02.txt               
diff --ignore-all-space correct02.txt output02.txt
./parse < test03.txt > output03.txt               
diff --ignore-all-space correct03.txt output03.txt
./parse < test04.txt > output04.txt               
diff --ignore-all-space correct04.txt output04.txt
```

### Error Detector
- test from Michael's mail
```
sum := ( x x < yy )
(program
[ (:= "sum"Expression Exception: error in line number: 1
follow:  in lineno: 1, token: lt
Expression Exception: error in line number: 2
deleting token: eof, error in lineno: 2

match error in line: 2 , get eof, insert: rparen
  (< (id "x") (id "yy")))
]
)
```
- test06
```
read a read b read c write ( a * ( b + c
(program
[ (read "a")
(read "b")
(read "c")
(writeExpression Exception: error in line number: 1
discard token: eof, error in lineno: 1
Expression Exception: error in line number: 1
discard token: eof, error in lineno: 1
  (* (id "a")  (+ (id "b") (id "c"))))
]
)
```
- test07
```
Y := (A * X write A * B
(*  (* (id "A") (id "X")) (id "B")))
```
- test08
```
(program
[ (read "a")
(read "b")
(:= "Y"Relation Exception , line number: 1
first: in lineno: 1, token: a
 (* (id "a") (id "b")))
]
)
```
- test09
```
(program                                                                                 
[ (read "a")                                                                             
(write  (add (id "a")  (mul (num "4") (num "5"))))                                      
(write (num "3"))                                                                     
]                                                                   
)
```
### For More Details such as context specific error detction
please see README.pdf

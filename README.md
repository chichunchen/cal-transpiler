# A2

### TODO
- [ ] Translate the code to c++
- [ ] Write test cases with Makefile
    - [ ] testXX.txt as the code of calculator language
    - [ ] outputXX.txt as the output AST for the correspondent testXX.txt
- [ ] Extend the language with if and do/check statements
- [ ] Implement exception-based syntax error recovery, as described in Section 2.3.5 on the textbook’s companion site. At the least, you should attach handlers to statements, relations, and expressions. 
- [ ] Output a syntax tree with the structure suggested

### Extended Grammar
- Here the new nonterminal R is meant to suggest a “relation.”  As in C, a value of 0 is taken to be false; anything else is true.
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
$$
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

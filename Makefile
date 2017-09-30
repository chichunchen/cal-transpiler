# Trivial makefile for the calculator scanner/parser.
# Depends on default (built-in) rules for C compilation.

# Note that rule for goal (parse) must be the first one in this file.

CC = g++
CFLAGS = -g -Wall -O2

parse: parse.o scan.o ast.o semantic.o
	$(CC) $(CFLAGS) -o parse parse.o scan.o ast.o semantic.o

clean:
	rm *.o parse

test01:
	./parse < test01.txt > output01.txt

test02:
	./parse < test02.txt > output02.txt

test03:
	./parse < test03.txt > output03.txt

test04:
	./parse < test04.txt > output04.txt
	diff --ignore-all-space correct04.txt output04.txt

tests: test01 test02 test03 test04

parse.o: scan.h debug.h
scan.o: scan.h debug.h
ast.o: ast.h scan.h debug.h
semantic.o: scan.h debug.h semantic.h

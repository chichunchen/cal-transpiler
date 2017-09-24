# Trivial makefile for the calculator scanner/parser.
# Depends on default (built-in) rules for C compilation.

# Note that rule for goal (parse) must be the first one in this file.

CC = g++
CFLAGS = -g -Wall -O2

parse: parse.o scan.o
	$(CC) $(CFLAGS) -o parse parse.o scan.o

clean:
	rm *.o parse

test01:
	./parse < test01.txt > output01.txt
	diff correct01.txt output01.txt

test02:
	./parse < test02.txt > output02.txt
	diff correct02.txt output02.txt

test03:
	./parse < test03.txt > output03.txt
	diff correct03.txt output03.txt

tests: test01 test02 test03

parse.o: scan.h
scan.o: scan.h

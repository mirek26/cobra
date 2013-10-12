CC = clang++
CCFLAGS = -Wall -pedantic -std=c++11
FLEX = flex
BISON = bison -d

default: cobra

cobra: formula.o cobra.tab.o cobra.yy.o
	$(CC) formula.o cobra.tab.o cobra.yy.o -o cobra -ll 

# bison 
cobra.tab.cpp: cobra.ypp
	$(BISON) $<

# flex
cobra.yy.c: cobra.l
	$(FLEX) -o $@ $<

cobra.yy.o: cobra.yy.c
	# suppress all warnings (compiling c as c++ etc.)
	$(CC) -c -std=c++11 -w $< -o $@

# general
%.o: %.cpp
	$(CC) -c $(CCFLAGS) $< -o $@

clean: 
	rm -rf *.o cobra.tab.cpp cobra.tab.hpp cobra.yy.c cobra

CC = g++ -g
CCFLAGS = -Wall -pedantic -std=c++11
FLEX = flex
BISON = bison -d

default: clean cobra

cobra: formula.o ast-manager.o util.o cobra.tab.o cobra.yy.o main.o
	$(CC) formula.o ast-manager.o util.o cobra.tab.o cobra.yy.o main.o -o cobra -ll

main.cpp: cobra.tab.cpp

# bison
cobra.tab.cpp: cobra.ypp
	$(BISON) $<

# flex
cobra.yy.c: cobra.l
	$(FLEX) -o $@ $<

cobra.yy.o: cobra.yy.c
	@# suppress all warnings (compiling c as c++ etc.)
	$(CC) -c -std=c++11 -w $< -o $@

# general
%.o: %.cpp %.h
	$(CC) -c $(CCFLAGS) $< -o $@

%.o: %.cpp
	$(CC) -c $(CCFLAGS) $< -o $@

cobra.tab.o: cobra.tab.cpp cobra.ypp
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -rf *.o cobra.tab.cpp cobra.tab.hpp cobra.yy.c cobra

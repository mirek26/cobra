CC = g++
CCFLAGS = -g -Wall -pedantic -std=c++11
FLEX = flex
BISON = bison -d

# file names
EXEC = cobra
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o) cobra.yy.o

default: cobra
cobra: $(OBJECTS)
	$(CC) $(OBJECTS) -L sat-solvers/picosat -l picosat -ll -o $(EXEC)

# parser
parser: cobra.tab.cpp cobra.yy.c
cobra.tab.cpp: cobra.ypp
	$(BISON) $<
cobra.tab.o: cobra.tab.cpp cobra.ypp
cobra.yy.c: cobra.l
	$(FLEX) -o $@ $<
cobra.yy.o: cobra.yy.c cobra.l
	@# suppress all warnings (compiling c as c++ etc.)
	$(CC) -c -std=c++11 -w $< -o $@

# general
%.o: %.cpp %.h
	$(CC) -c $(CCFLAGS) $< -o $@
%.o: %.cpp
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -rf *.o cobra

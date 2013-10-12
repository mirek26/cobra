CXX=clang++
CXXFLAGS=-c -Wall -std=c++11

cobra: formula.o cobra.tab.o lex.yy.o 
	$(CXX) cobra.tab.o formula.o lex.yy.o -o cobra -ll 

cobra.tab.c: cobra.y
	bison -d cobra.y
cobra.tab.h: cobra.tab.h

cobra.tab.o: cobra.tab.c
	$(CXX) $(CXXFLAGS) cobra.tab.c

lex.yy.c: cobra.l
	flex cobra.l

lex.yy.o: lex.yy.c cobra.tab.h
	$(CXX) $(CXXFLAGS) lex.yy.c

formula.o: formula.cpp
	$(CXX) $(CXXFLAGS) formula.cpp

clean: 
	rm -rf *.o cobra.tab.c cobra.tab.h cobra

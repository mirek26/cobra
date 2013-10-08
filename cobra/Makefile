CXX=clang++
CXXFLAGS=-c -Wall -std=c++11

cobra: formula.o cobra.tab.o
	$(CXX) cobra.tab.o formula.o -o cobra

cobra.tab.o: cobra.y
	bison cobra.y
	$(CXX) $(CXXFLAGS) cobra.tab.c

formula.o: formula.cpp
	$(CXX) $(CXXFLAGS) formula.cpp

clean: 
	rm -rf *.o cobra.tab.c cobra

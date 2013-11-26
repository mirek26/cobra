CC = clang++
CCFLAGS = -g -Wall -Wextra -pedantic -pthread -std=c++11
FLEX = flex
BISON = bison -d

# file names
EXEC = cobra
SOURCES = $(filter-out test.cpp main.cpp, $(wildcard *.cpp))
OBJECTS = $(SOURCES:.cpp=.o) cobra.yy.o

default: cobra
cobra: $(OBJECTS) main.o
	$(CC) $(OBJECTS) main.o -L sat-solvers/picosat -l picosat -ll -o $(EXEC)

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

# gtest
GTEST_DIR = ./test
IGNFLAGS += -isystem $(GTEST_DIR)/include
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

$(GTEST_DIR)/gtest-all.o: $(GTEST_SRCS_)
	$(CC) $(IGNFLAGS) -I$(GTEST_DIR) $(CCFLAGS) -c $(GTEST_DIR)/src/gtest-all.cc -o $@

$(GTEST_DIR)/gtest_main.o: $(GTEST_SRCS_)
	$(CC) $(IGNFLAGS) -I$(GTEST_DIR) $(CCFLAGS) -c $(GTEST_DIR)/src/gtest_main.cc -o $@

$(GTEST_DIR)/gtest.a: $(GTEST_DIR)/gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

$(GTEST_DIR)/gtest_main.a: $(GTEST_DIR)/gtest-all.o $(GTEST_DIR)/gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

$(GTEST_DIR)/test.o: $(GTEST_DIR)/test.cpp *.h $(GTEST_HEADERS)
	$(CC) $(IGNFLAGS) $(CCFLAGS) -c $(GTEST_DIR)/test.cpp -o $@

$(GTEST_DIR)/test: $(GTEST_DIR)/test.o $(OBJECTS) $(GTEST_DIR)/gtest_main.a
	$(CC) $(IGNFLAGS) $^ -L sat-solvers/picosat -lpthread -lpicosat -ll -o $@

test: $(GTEST_DIR)/test
	./test/test

test-clean:
	rm -rf $(GTEST_DIR)/*.o $(GTEST_DIR)/*.a $(GTEST_DIR)/test

.PHONY: test

# clean
clean:
	rm -rf *.o cobra


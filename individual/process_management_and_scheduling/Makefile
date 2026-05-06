# Subsystem A: Process Management & CPU Scheduling
# Standalone Makefile (Part I)

CXX      = g++
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c++17 -g -Iinclude

SRC     = src/main.cpp src/sched.cpp
TARGET  = sched_demo

TEST_SRC    = tests/test_sched.cpp src/sched.cpp
TEST_TARGET = test_sched

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(TEST_TARGET)

test: $(TEST_TARGET)
	@echo "Running Subsystem A tests..."
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

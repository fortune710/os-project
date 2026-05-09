# Subsystem B: Memory Management & Virtual Memory
# Standalone Makefile (Part I)

CXX      = g++
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c++17 -g -Iinclude

SRC     = src/main.cpp src/mem.cpp
TARGET  = mem_demo

TEST_SRC    = tests/test_mem.cpp src/mem.cpp
TEST_TARGET = test_mem

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(TEST_TARGET)

test: $(TEST_TARGET)
	@echo "Running Subsystem B tests..."
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

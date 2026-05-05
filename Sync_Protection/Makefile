# Subsystem C: Synchronization & Protection
# Standalone Makefile (Part I)

CXX      = g++
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c++17 -g -Iinclude

# Sync depends on sched for PCB role lookups
SRC     = src/main.cpp src/sync.cpp src/sched.cpp
TARGET  = sync_demo

TEST_SRC    = tests/test_sync.cpp src/sync.cpp src/sched.cpp
TEST_TARGET = test_sync

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(TEST_TARGET)

test: $(TEST_TARGET)
	@echo "Running Subsystem C tests..."
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

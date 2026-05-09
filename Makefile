# Subsystem C: Synchronization & Protection
# Standalone Makefile (Part I)

CXX      = g++
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c++17 -g -Iinclude

# Sync depends on sched for PCB role lookups
SRC      = src/main.cpp src/sync/sync.cpp src/sched/sched.cpp src/mem/mem.cpp
TARGET   = sync_demo
EXE      = $(TARGET)$(if $(filter Windows%,$(OS)),.exe,)

TEST_SRC    = tests/test_sync.cpp src/sync/sync.cpp src/sched/sched.cpp src/mem/mem.cpp
TEST_TARGET = test_sync
TEST_EXE    = $(TEST_TARGET)$(if $(filter Windows%,$(OS)),.exe,)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(TARGET).exe $(TEST_TARGET) $(TEST_TARGET).exe

test: $(TEST_TARGET)
	@echo "Running Subsystem C tests..."
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

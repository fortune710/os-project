CC ?= gcc
CXX ?= g++
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Iinclude
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?=

SRC_C :=
SRC_CPP := \
  src/main.cpp \
  src/scheduler/scheduler.cpp \
  src/mem/mem.cpp \
  src/sync/sync.cpp

OBJ_C := $(SRC_C:.c=.o)
OBJ_CPP := $(SRC_CPP:.cpp=.o)
OBJ := $(OBJ_C) $(OBJ_CPP)

all: moss

moss: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) moss

.PHONY: all clean

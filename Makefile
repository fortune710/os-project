CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?=

SRC := \
  src/main.c \
  src/sched/sched.c \
  src/mem/mem.c \
  src/sync/sync.c

OBJ := $(SRC:.c=.o)

all: moss

moss: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) moss

.PHONY: all clean

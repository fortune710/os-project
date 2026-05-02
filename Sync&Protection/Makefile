CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude

all: moss_sim

moss_sim: src/main.c src/sync/sync.c
	$(CC) $(CFLAGS) src/main.c src/sync/sync.c -o moss_sim

clean:
	rm -f moss_sim src/sync/*.o

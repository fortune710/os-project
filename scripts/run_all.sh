#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$ROOT_DIR"
make clean
make
./moss

if [ -f tests/basic_tests.c ]; then
  gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude \
    tests/basic_tests.c src/sched/sched.c src/mem/mem.c src/sync/sync.c \
    -o tests/basic_tests
  ./tests/basic_tests
fi

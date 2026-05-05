#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$ROOT_DIR"
make
./moss

# Build and run basic tests (if desired)
if [ -f tests/basic_tests.cpp ]; then
  g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude \
    tests/basic_tests.cpp src/scheduler/scheduler.cpp src/scheduler/algorithms.cpp src/mem/mem.cpp src/sync/sync.cpp \
    -o tests/basic_tests
  ./tests/basic_tests
fi

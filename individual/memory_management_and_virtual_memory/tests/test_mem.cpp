/*
 * test_mem.cpp - Unit tests for Subsystem B
 * Memory Management & Virtual Memory
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include "moss_common.h"
#include "moss_mem.h"

static int tests_run = 0, tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; std::printf("  [PASS] %s\n", msg); } \
    else { std::printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } \
} while(0)

void moss_log(LogLevel, const char *, ...) { /* silent */ }

static void test_allocation(void) {
    std::printf("\n  ── Memory Allocation ──\n");
    mem_init();

    TEST_ASSERT(mem_allocate(0, 4) == 0, "Allocate 4 pages");
    TEST_ASSERT(mem_allocate(0, 2) < 0, "Double allocation rejected");
    TEST_ASSERT(mem_allocate(-1, 4) < 0, "Negative PID rejected");
    TEST_ASSERT(mem_allocate(1, 0) < 0, "Zero pages rejected");

    mem_cleanup();
}

static void test_access(void) {
    std::printf("\n  ── Memory Access ──\n");
    mem_init();
    mem_allocate(0, 4);

    int p1 = mem_access(0, 0x0010);
    TEST_ASSERT(p1 >= 0, "First access (page fault) returns physical address");

    int p2 = mem_access(0, 0x0050);
    TEST_ASSERT(p2 >= 0, "Second access same page (hit)");

    int p3 = mem_access(0, 0x0110);
    TEST_ASSERT(p3 >= 0, "Different page access");

    TEST_ASSERT(mem_access(0, 0x0F00) < 0, "Out of range rejected");
    TEST_ASSERT(mem_access(99, 0x0010) < 0, "Unknown PID rejected");

    mem_cleanup();
}

static void test_fifo_replacement(void) {
    std::printf("\n  ── FIFO Page Replacement ──\n");
    mem_init();
    mem_set_replacement("FIFO");
    mem_allocate(0, 16);

    for (int i = 0; i < MAX_FRAMES; i++) {
        int r = mem_access(0, static_cast<uint16_t>(i * PAGE_SIZE + 10));
        TEST_ASSERT(r >= 0, "Fill frame");
    }

    int r = mem_access(0, static_cast<uint16_t>(8 * PAGE_SIZE + 10));
    TEST_ASSERT(r >= 0, "FIFO eviction succeeds");

    r = mem_access(0, 0x0010);
    TEST_ASSERT(r >= 0, "Evicted page reloads");

    mem_cleanup();
}

static void test_lru_replacement(void) {
    std::printf("\n  ── LRU Page Replacement ──\n");
    mem_init();
    mem_set_replacement("LRU");
    mem_allocate(0, 16);

    for (int i = 0; i < MAX_FRAMES; i++) {
        mem_access(0, static_cast<uint16_t>(i * PAGE_SIZE + 10));
    }
    mem_access(0, 0x0010); /* Make page 0 MRU */

    int r = mem_access(0, static_cast<uint16_t>(8 * PAGE_SIZE + 10));
    TEST_ASSERT(r >= 0, "LRU eviction succeeds");

    mem_cleanup();
}

static void test_free(void) {
    std::printf("\n  ── Memory Free ──\n");
    mem_init();
    mem_allocate(0, 4);
    mem_access(0, 0x0010);

    TEST_ASSERT(mem_free(0) == 0, "Free succeeds");
    TEST_ASSERT(mem_free(0) < 0, "Double free rejected");
    TEST_ASSERT(mem_access(0, 0x0010) < 0, "Access after free rejected");

    mem_cleanup();
}

int main(void) {
    std::printf("\n  ═══ Subsystem B: Memory Management Tests ═══\n");

    test_allocation();
    test_access();
    test_fifo_replacement();
    test_lru_replacement();
    test_free();

    std::printf("\n  Results: %d/%d passed\n\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}

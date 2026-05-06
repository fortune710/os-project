/*
 * test_sched.cpp - Unit tests for Subsystem A
 * Process Management & CPU Scheduling
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include "moss_common.h"
#include "moss_sched.h"

static int tests_run = 0, tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; std::printf("  [PASS] %s\n", msg); } \
    else { std::printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } \
} while(0)

void moss_log(LogLevel, const char *, ...) { /* silent */ }

static void test_process_creation(void) {
    std::printf("\n  ── Process Creation ──\n");
    sched_init();

    int pid1 = sched_create_process("P1", 5, 0, 1, ROLE_USER);
    TEST_ASSERT(pid1 >= 0, "Create process returns valid PID");

    int pid2 = sched_create_process("P2", 3, 2, 2, ROLE_ADMIN);
    TEST_ASSERT(pid2 >= 0, "Create second process");
    TEST_ASSERT(pid2 != pid1, "PIDs are unique");

    PCB *p = sched_get_process(pid1);
    TEST_ASSERT(p != nullptr, "Get process returns valid pointer");
    TEST_ASSERT(p->burst_time == 5, "Burst time correct");
    TEST_ASSERT(p->arrival_time == 0, "Arrival time correct");
    TEST_ASSERT(p->state == PROC_READY, "Initial state is READY");

    TEST_ASSERT(sched_create_process(nullptr, 5, 0, 0, ROLE_USER) < 0, "nullptr name rejected");
    TEST_ASSERT(sched_create_process("X", -1, 0, 0, ROLE_USER) < 0, "Negative burst rejected");

    sched_cleanup();
}

static void test_process_termination(void) {
    std::printf("\n  ── Process Termination ──\n");
    sched_init();

    int pid = sched_create_process("Kill", 5, 0, 0, ROLE_USER);
    TEST_ASSERT(pid >= 0, "Create process");
    TEST_ASSERT(sched_terminate_process(pid) == 0, "Terminate succeeds");
    TEST_ASSERT(sched_get_process(pid) == nullptr, "Process removed");
    TEST_ASSERT(sched_terminate_process(999) < 0, "Non-existent PID returns error");

    sched_cleanup();
}

static void test_fcfs(void) {
    std::printf("\n  ── FCFS Scheduling ──\n");
    sched_init();

    sched_create_process("P1", 6, 0, 0, ROLE_USER);
    sched_create_process("P2", 4, 1, 0, ROLE_USER);
    sched_create_process("P3", 2, 2, 0, ROLE_USER);

    TEST_ASSERT(sched_run_fcfs() == 0, "FCFS completes");

    sched_cleanup();
}

static void test_rr(void) {
    std::printf("\n  ── Round Robin Scheduling ──\n");
    sched_init();

    sched_create_process("P1", 5, 0, 0, ROLE_USER);
    sched_create_process("P2", 3, 0, 0, ROLE_USER);
    sched_create_process("P3", 1, 0, 0, ROLE_USER);

    TEST_ASSERT(sched_run_rr(2) == 0, "RR completes");

    sched_init();
    sched_create_process("P1", 5, 0, 0, ROLE_USER);
    TEST_ASSERT(sched_run_rr(0) < 0, "Quantum=0 rejected");
    TEST_ASSERT(sched_run_rr(-1) < 0, "Negative quantum rejected");

    sched_cleanup();
}

static void test_empty(void) {
    std::printf("\n  ── Empty Scheduler ──\n");
    sched_init();
    TEST_ASSERT(sched_run_fcfs() < 0, "FCFS with no processes errors");
    TEST_ASSERT(sched_run_rr(2) < 0, "RR with no processes errors");
    sched_cleanup();
}

int main(void) {
    std::printf("\n  ═══ Subsystem A: Scheduling Tests ═══\n");

    test_process_creation();
    test_process_termination();
    test_fcfs();
    test_rr();
    test_empty();

    std::printf("\n  Results: %d/%d passed\n\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}

/*
 * test_sync.cpp - Unit tests for Subsystem C
 * Synchronization & Protection
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include "moss_common.h"
#include "moss_sched.h"
#include "moss_sync.h"

static int tests_run = 0, tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; std::printf("  [PASS] %s\n", msg); } \
    else { std::printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } \
} while(0)

void moss_log(LogLevel, const char *, ...) { /* silent */ }

static void test_mutex(void) {
    std::printf("\n  ── Mutex Lock/Unlock ──\n");
    sched_init(); sync_init();

    int p0 = sched_create_process("P0", 5, 0, 0, ROLE_USER);
    int p1 = sched_create_process("P1", 5, 0, 0, ROLE_USER);

    int m = sync_mutex_create("test_lock");
    TEST_ASSERT(m >= 0, "Create mutex");
    TEST_ASSERT(sync_mutex_lock(m, p0) == 0, "First lock");
    TEST_ASSERT(sync_mutex_lock(m, p0) == MOSS_ERR_BUSY, "Re-entrant blocked");
    TEST_ASSERT(sync_mutex_lock(m, p1) == MOSS_ERR_BUSY, "Second process blocked");
    TEST_ASSERT(sync_mutex_unlock(m, p1) == MOSS_ERR_DENIED, "Non-owner denied");
    TEST_ASSERT(sync_mutex_unlock(m, p0) == 0, "Owner unlock (handoff)");
    TEST_ASSERT(sync_mutex_unlock(m, p1) == 0, "Handed-off owner unlock");
    TEST_ASSERT(sync_mutex_lock(999, p0) < 0, "Invalid mutex ID");

    sync_cleanup(); sched_cleanup();
}

static void test_semaphore(void) {
    std::printf("\n  ── Semaphore ──\n");
    sched_init(); sync_init();

    int p0 = sched_create_process("P0", 5, 0, 0, ROLE_USER);
    int p1 = sched_create_process("P1", 5, 0, 0, ROLE_USER);

    int s = sync_sem_create("test_sem", 2);
    TEST_ASSERT(s >= 0, "Create semaphore");
    TEST_ASSERT(sync_sem_wait(s, p0) == 0, "First wait (val=1)");
    TEST_ASSERT(sync_sem_wait(s, p1) == 0, "Second wait (val=0)");
    TEST_ASSERT(sync_sem_wait(s, p0) == MOSS_ERR_BUSY, "Third wait blocks");
    TEST_ASSERT(sync_sem_signal(s, p1) == 0, "Signal wakes waiter");
    TEST_ASSERT(sync_sem_wait(999, p0) < 0, "Invalid semaphore ID");

    sync_cleanup(); sched_cleanup();
}

static void test_access_control(void) {
    std::printf("\n  ── Access Control ──\n");
    sched_init(); sync_init();

    int admin = sched_create_process("Admin", 5, 0, 0, ROLE_ADMIN);
    int user = sched_create_process("User", 5, 0, 0, ROLE_USER);

    TEST_ASSERT(sync_check_permission(admin, "memory", "execute") == 0, "Admin: execute memory");
    TEST_ASSERT(sync_check_permission(admin, "process", "write") == 0, "Admin: write process");
    TEST_ASSERT(sync_check_permission(user, "memory", "read") == 0, "User: read memory");
    TEST_ASSERT(sync_check_permission(user, "memory", "execute") == MOSS_ERR_DENIED, "User: execute denied");
    TEST_ASSERT(sync_check_permission(user, "process", "write") == MOSS_ERR_DENIED, "User: write denied");

    sync_set_role(user, ROLE_ADMIN);
    TEST_ASSERT(sync_check_permission(user, "memory", "execute") == 0, "After promotion: granted");
    TEST_ASSERT(sync_check_permission(999, "memory", "read") < 0, "Unknown PID");

    sync_cleanup(); sched_cleanup();
}

static void test_deadlock(void) {
    std::printf("\n  ── Deadlock Detection ──\n");
    sched_init(); sync_init();

    int p0 = sched_create_process("P0", 5, 0, 0, ROLE_USER);
    int p1 = sched_create_process("P1", 5, 0, 0, ROLE_USER);

    TEST_ASSERT(sync_detect_deadlock() == 0, "No deadlock initially");

    int m1 = sync_mutex_create("r1");
    int m2 = sync_mutex_create("r2");
    sync_mutex_lock(m1, p0);
    sync_mutex_lock(m2, p1);
    sync_mutex_lock(m2, p0);
    sync_mutex_lock(m1, p1);

    TEST_ASSERT(sync_detect_deadlock() == MOSS_ERR_DEADLOCK, "Deadlock detected");

    sync_cleanup(); sched_cleanup();
}

static void test_producer_consumer(void) {
    std::printf("\n  ── Producer-Consumer ──\n");
    sync_init();

    TEST_ASSERT(sync_run_producer_consumer(3, 5) == 0, "PC simulation completes");
    TEST_ASSERT(sync_run_producer_consumer(0, 5) < 0, "Zero buffer rejected");
    TEST_ASSERT(sync_run_producer_consumer(3, 0) < 0, "Zero items rejected");

    sync_cleanup();
}

int main(void) {
    std::printf("\n  ═══ Subsystem C: Synchronization Tests ═══\n");

    test_mutex();
    test_semaphore();
    test_access_control();
    test_deadlock();
    test_producer_consumer();

    std::printf("\n  Results: %d/%d passed\n\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}

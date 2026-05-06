/*
 * main.cpp - Standalone driver for Subsystem C
 * Synchronization & Protection
 *
 * Individual Part I deliverable. This standalone program
 * demonstrates synchronization and access control independently.
 *
 * Includes a minimal process manager so that access control
 * permission checks can look up process roles.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <cctype>
#include <string>

#include "moss_common.h"
#include "moss_sched.h"
#include "moss_sync.h"

/* Logging implementation */
void moss_log(LogLevel level, const char *fmt, ...) {
    const char *lvl = (level == LOG_INFO) ? "INFO" :
                      (level == LOG_WARN) ? "WARN" : "ERROR";
    std::time_t now = std::time(nullptr);
    struct tm *t = std::localtime(&now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", t);
    std::printf("  [%s] [%s] ", buf, lvl);
    va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    va_end(args);
    std::printf("\n");
}

static char* trim(char *str) {
    while (std::isspace(static_cast<unsigned char>(*str))) str++;
    if (*str == 0) return str;
    char *end = str + std::strlen(str) - 1;
    while (end > str && std::isspace(static_cast<unsigned char>(*end))) end--;
    *(end + 1) = '\0';
    return str;
}

static void print_help(void) {
    std::printf("\n");
    std::printf("  ╔══════════════════════════════════════════════════════════╗\n");
    std::printf("  ║   Subsystem C: Synchronization & Protection            ║\n");
    std::printf("  ║                   Command Reference                     ║\n");
    std::printf("  ╚══════════════════════════════════════════════════════════╝\n\n");
    std::printf("  create_process <name> <burst> <role>   Create a process (admin|user)\n");
    std::printf("  create_mutex <name>                    Create a mutex\n");
    std::printf("  lock <id> <pid>                        Lock a mutex\n");
    std::printf("  unlock <id> <pid>                      Unlock a mutex\n");
    std::printf("  create_sem <name> <value>              Create a semaphore\n");
    std::printf("  sem_wait <id> <pid>                    Semaphore wait\n");
    std::printf("  sem_signal <id> <pid>                  Semaphore signal\n");
    std::printf("  run_pc <buffer> <items>                Producer-Consumer sim\n");
    std::printf("  check_perm <pid> <resource> <action>   Check permission\n");
    std::printf("  set_role <pid> <admin|user>             Change role\n");
    std::printf("  deadlock                               Detect deadlock\n");
    std::printf("  state                                  Show sync state\n");
    std::printf("  demo                                   Run demo scenario\n");
    std::printf("  help                                   Show this help\n");
    std::printf("  exit                                   Exit\n\n");
}

static void run_demo(void) {
    sched_cleanup(); sched_init();
    sync_cleanup(); sync_init();

    std::printf("\n  ── Demo: Mutex Lock/Unlock & Handoff ─────────────────────\n");
    int p0 = sched_create_process("Admin", 5, 0, 0, ROLE_ADMIN);
    int p1 = sched_create_process("User1", 5, 0, 0, ROLE_USER);

    int m = sync_mutex_create("resource_lock");
    sync_mutex_lock(m, p0);    /* P0 acquires */
    sync_mutex_lock(m, p1);    /* P1 blocked */
    sync_mutex_unlock(m, p0);  /* Hands to P1 */
    sync_mutex_unlock(m, p1);  /* P1 releases */
    std::printf("\n");
    sync_print_state();

    std::printf("\n  ── Demo: Semaphore & Producer-Consumer ────────────────────\n");
    sync_run_producer_consumer(3, 4);

    std::printf("\n  ── Demo: Access Control ──────────────────────────────────\n");
    std::printf("  Checking permissions:\n");
    sync_check_permission(p0, "memory", "execute");   /* Admin: granted */
    sync_check_permission(p1, "memory", "execute");   /* User: denied */
    sync_check_permission(p1, "memory", "read");      /* User: granted */
    std::printf("\n  Elevating User1 to Admin:\n");
    sync_set_role(p1, ROLE_ADMIN);
    sync_check_permission(p1, "memory", "execute");   /* Now granted */

    std::printf("\n  ── Demo: Deadlock Detection ───────────────────────────────\n");
    int m1 = sync_mutex_create("res_A");
    int m2 = sync_mutex_create("res_B");
    sync_mutex_lock(m1, p0);
    sync_mutex_lock(m2, p1);
    sync_mutex_lock(m2, p0);  /* P0 waits for P1 */
    sync_mutex_lock(m1, p1);  /* P1 waits for P0 → DEADLOCK */
    std::printf("\n  Running deadlock detection:\n");
    sync_detect_deadlock();
    std::printf("\n");
}

int main(void) {
    sched_init();
    sync_init();

    std::printf("\n");
    std::printf("  ╔══════════════════════════════════════════════════════════╗\n");
    std::printf("  ║   Subsystem C: Synchronization & Protection            ║\n");
    std::printf("  ╚══════════════════════════════════════════════════════════╝\n");
    std::printf("\n  Type 'help' for commands.\n\n");

    char line[256];
    while (true) {
        std::printf("sync> ");
        std::fflush(stdout);
        if (std::fgets(line, sizeof(line), stdin) == nullptr) break;
        line[std::strcspn(line, "\n")] = '\0';

        char *trimmed = trim(line);
        if (std::strlen(trimmed) == 0) continue;

        char *cmd = std::strtok(trimmed, " \t");
        std::string command(cmd);

        if (command == "create_process") {
            char *name = std::strtok(nullptr, " \t");
            char *burst_s = std::strtok(nullptr, " \t");
            char *role_s = std::strtok(nullptr, " \t");
            if (!name || !burst_s || !role_s) {
                std::printf("  Usage: create_process <name> <burst> <role>\n"); continue;
            }
            std::string rs(role_s);
            UserRole role = (rs == "admin") ? ROLE_ADMIN : ROLE_USER;
            int pid = sched_create_process(name, std::atoi(burst_s), 0, 0, role);
            if (pid >= 0) std::printf("  Process PID %d created (%s)\n", pid, role_s);
            else std::printf("  Error (%d)\n", pid);
        }
        else if (command == "create_mutex") {
            char *n = std::strtok(nullptr, " \t");
            if (!n) { std::printf("  Usage: create_mutex <name>\n"); continue; }
            int id = sync_mutex_create(n);
            std::printf(id >= 0 ? "  Mutex ID %d created\n" : "  Error (%d)\n", id);
        }
        else if (command == "lock") {
            char *id_s = std::strtok(nullptr, " \t");
            char *pid_s = std::strtok(nullptr, " \t");
            if (!id_s || !pid_s) { std::printf("  Usage: lock <id> <pid>\n"); continue; }
            int r = sync_mutex_lock(std::atoi(id_s), std::atoi(pid_s));
            if (r == 0) std::printf("  Locked\n");
            else if (r == MOSS_ERR_BUSY) std::printf("  Busy — queued\n");
            else std::printf("  Error (%d)\n", r);
        }
        else if (command == "unlock") {
            char *id_s = std::strtok(nullptr, " \t");
            char *pid_s = std::strtok(nullptr, " \t");
            if (!id_s || !pid_s) { std::printf("  Usage: unlock <id> <pid>\n"); continue; }
            int r = sync_mutex_unlock(std::atoi(id_s), std::atoi(pid_s));
            if (r == 0) std::printf("  Unlocked\n");
            else if (r == MOSS_ERR_DENIED) std::printf("  Denied — not owner\n");
            else std::printf("  Error (%d)\n", r);
        }
        else if (command == "create_sem") {
            char *n = std::strtok(nullptr, " \t");
            char *v = std::strtok(nullptr, " \t");
            if (!n || !v) { std::printf("  Usage: create_sem <name> <value>\n"); continue; }
            int id = sync_sem_create(n, std::atoi(v));
            std::printf(id >= 0 ? "  Semaphore ID %d created\n" : "  Error (%d)\n", id);
        }
        else if (command == "sem_wait") {
            char *id_s = std::strtok(nullptr, " \t");
            char *pid_s = std::strtok(nullptr, " \t");
            if (!id_s || !pid_s) { std::printf("  Usage: sem_wait <id> <pid>\n"); continue; }
            int r = sync_sem_wait(std::atoi(id_s), std::atoi(pid_s));
            if (r == 0) std::printf("  Wait succeeded\n");
            else if (r == MOSS_ERR_BUSY) std::printf("  Blocked\n");
            else std::printf("  Error (%d)\n", r);
        }
        else if (command == "sem_signal") {
            char *id_s = std::strtok(nullptr, " \t");
            char *pid_s = std::strtok(nullptr, " \t");
            if (!id_s || !pid_s) { std::printf("  Usage: sem_signal <id> <pid>\n"); continue; }
            int r = sync_sem_signal(std::atoi(id_s), std::atoi(pid_s));
            std::printf(r == 0 ? "  Signaled\n" : "  Error (%d)\n", r);
        }
        else if (command == "run_pc") {
            char *b = std::strtok(nullptr, " \t");
            char *n = std::strtok(nullptr, " \t");
            if (!b || !n) { std::printf("  Usage: run_pc <buffer> <items>\n"); continue; }
            sync_run_producer_consumer(std::atoi(b), std::atoi(n));
        }
        else if (command == "check_perm") {
            char *pid_s = std::strtok(nullptr, " \t");
            char *res = std::strtok(nullptr, " \t");
            char *act = std::strtok(nullptr, " \t");
            if (!pid_s || !res || !act) {
                std::printf("  Usage: check_perm <pid> <resource> <action>\n"); continue;
            }
            int r = sync_check_permission(std::atoi(pid_s), res, act);
            std::printf(r == 0 ? "  GRANTED\n" : "  DENIED\n");
        }
        else if (command == "set_role") {
            char *pid_s = std::strtok(nullptr, " \t");
            char *role_s = std::strtok(nullptr, " \t");
            if (!pid_s || !role_s) { std::printf("  Usage: set_role <pid> <admin|user>\n"); continue; }
            std::string rs(role_s);
            UserRole role = (rs == "admin") ? ROLE_ADMIN : ROLE_USER;
            int r = sync_set_role(std::atoi(pid_s), role);
            std::printf(r == 0 ? "  Role updated\n" : "  Error (%d)\n", r);
        }
        else if (command == "deadlock") { sync_detect_deadlock(); }
        else if (command == "state") { sync_print_state(); }
        else if (command == "demo") { run_demo(); }
        else if (command == "help") { print_help(); }
        else if (command == "exit" || command == "quit") { break; }
        else { std::printf("  Unknown command. Type 'help'.\n"); }
    }

    sync_cleanup();
    sched_cleanup();
    std::printf("  Goodbye!\n");
    return 0;
}

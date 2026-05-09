/*
 * main.cpp - Standalone driver for Subsystem A
 * Process Management & CPU Scheduling
 *
 * Individual Part I deliverable. This standalone program
 * demonstrates scheduling algorithms independently.
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

/* Trim whitespace */
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
    std::printf("  ║   Subsystem A: Process Management & CPU Scheduling     ║\n");
    std::printf("  ║                   Command Reference                     ║\n");
    std::printf("  ╚══════════════════════════════════════════════════════════╝\n\n");
    std::printf("  create_process <name> <burst> <arrival> [priority]  Create a process\n");
    std::printf("  list_processes                                      List all processes\n");
    std::printf("  terminate <pid>                                     Terminate a process\n");
    std::printf("  schedule <FCFS|RR> [quantum]                        Run scheduling\n");
    std::printf("  gantt                                               Show Gantt chart\n");
    std::printf("  stats                                               Show statistics\n");
    std::printf("  demo                                                Run demo scenario\n");
    std::printf("  help                                                Show this help\n");
    std::printf("  exit                                                Exit\n\n");
}

static void run_demo(void) {
    std::printf("\n  ── Demo: FCFS Scheduling ──────────────────────────────────\n");
    sched_init();
    sched_create_process("WebServer", 6, 0, 1, ROLE_ADMIN);
    sched_create_process("Database", 4, 1, 2, ROLE_USER);
    sched_create_process("Logger", 2, 2, 3, ROLE_USER);
    sched_run_fcfs();
    sched_print_gantt();
    sched_print_stats();

    std::printf("\n  ── Demo: Round Robin (quantum=2) ──────────────────────────\n");
    sched_cleanup();
    sched_init();
    sched_create_process("WebServer", 6, 0, 1, ROLE_ADMIN);
    sched_create_process("Database", 4, 1, 2, ROLE_USER);
    sched_create_process("Logger", 2, 2, 3, ROLE_USER);
    sched_run_rr(2);
    sched_print_gantt();
    sched_print_stats();
    std::printf("\n");
}

int main(void) {
    sched_init();

    std::printf("\n");
    std::printf("  ╔══════════════════════════════════════════════════════════╗\n");
    std::printf("  ║   Subsystem A: Process Management & CPU Scheduling     ║\n");
    std::printf("  ╚══════════════════════════════════════════════════════════╝\n");
    std::printf("\n  Type 'help' for commands.\n\n");

    char line[256];
    while (true) {
        std::printf("sched> ");
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
            char *arr_s = std::strtok(nullptr, " \t");
            char *pri_s = std::strtok(nullptr, " \t");
            if (!name || !burst_s || !arr_s) {
                std::printf("  Usage: create_process <name> <burst> <arrival> [priority]\n");
                continue;
            }
            int pid = sched_create_process(name, std::atoi(burst_s), std::atoi(arr_s),
                                           pri_s ? std::atoi(pri_s) : 0, ROLE_USER);
            if (pid >= 0) std::printf("  Process created with PID %d\n", pid);
            else std::printf("  Error (code %d)\n", pid);
        }
        else if (command == "list_processes") { sched_list_processes(); }
        else if (command == "terminate") {
            char *pid_s = std::strtok(nullptr, " \t");
            if (!pid_s) { std::printf("  Usage: terminate <pid>\n"); continue; }
            int r = sched_terminate_process(std::atoi(pid_s));
            std::printf(r == 0 ? "  Terminated PID %s\n" : "  Not found\n", pid_s);
        }
        else if (command == "schedule") {
            char *algo = std::strtok(nullptr, " \t");
            if (!algo) { std::printf("  Usage: schedule <FCFS|RR> [quantum]\n"); continue; }
            std::string a(algo);
            int r;
            if (a == "FCFS") r = sched_run_fcfs();
            else if (a == "RR") {
                char *q = std::strtok(nullptr, " \t");
                r = sched_run_rr(q ? std::atoi(q) : 2);
            } else { std::printf("  Unknown: %s\n", algo); continue; }
            std::printf(r == 0 ? "  Done. Use 'gantt' and 'stats'.\n" : "  Failed (%d)\n", r);
        }
        else if (command == "gantt") { sched_print_gantt(); }
        else if (command == "stats") { sched_print_stats(); }
        else if (command == "demo") { run_demo(); }
        else if (command == "help") { print_help(); }
        else if (command == "exit" || command == "quit") { break; }
        else { std::printf("  Unknown command. Type 'help'.\n"); }
    }

    sched_cleanup();
    std::printf("  Goodbye!\n");
    return 0;
}

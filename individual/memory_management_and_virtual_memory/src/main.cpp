/*
 * main.cpp - Standalone driver for Subsystem B
 * Memory Management & Virtual Memory
 *
 * Individual Part I deliverable. This standalone program
 * demonstrates memory management independently.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <cctype>
#include <string>

#include "moss_common.h"
#include "moss_mem.h"

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
    std::printf("  ║   Subsystem B: Memory Management & Virtual Memory      ║\n");
    std::printf("  ║                   Command Reference                     ║\n");
    std::printf("  ╚══════════════════════════════════════════════════════════╝\n\n");
    std::printf("  alloc <pid> <pages>                Allocate pages for a process\n");
    std::printf("  access <pid> <hex_addr>            Access a logical address\n");
    std::printf("  free <pid>                         Free process memory\n");
    std::printf("  set_algo <FIFO|LRU>                Set replacement algorithm\n");
    std::printf("  page_table <pid>                   Show page table\n");
    std::printf("  frames                             Show physical frames\n");
    std::printf("  stats                              Show memory statistics\n");
    std::printf("  demo                               Run demo scenario\n");
    std::printf("  help                               Show this help\n");
    std::printf("  exit                               Exit\n\n");
}

static void run_demo(void) {
    std::printf("\n  ── Demo: Page Faults & Replacement ───────────────────────\n");
    mem_init();

    /* Simulate PID 0 with 4 pages */
    mem_allocate(0, 4);

    std::printf("\n  Accessing pages (should page fault on each new page):\n");
    mem_access(0, 0x0010);   /* Page 0 */
    mem_access(0, 0x0050);   /* Page 0 — hit */
    mem_access(0, 0x0120);   /* Page 1 */
    mem_access(0, 0x0210);   /* Page 2 */
    mem_access(0, 0x0310);   /* Page 3 */

    std::printf("\n");
    mem_print_page_table(0);
    std::printf("\n");
    mem_print_frames();
    std::printf("\n");
    mem_print_stats();

    /* Now demonstrate replacement — allocate more processes to fill frames */
    std::printf("\n  ── Demo: FIFO Page Replacement ────────────────────────────\n");
    mem_allocate(1, 8);
    std::printf("\n  Filling all 8 frames with PID 1 pages:\n");
    for (int i = 0; i < 8; i++) {
        mem_access(1, static_cast<uint16_t>(i * PAGE_SIZE + 10));
    }
    std::printf("\n  Accessing page 8 — triggers FIFO eviction:\n");
    mem_access(1, static_cast<uint16_t>(8 * PAGE_SIZE + 10));

    std::printf("\n");
    mem_print_frames();
    mem_print_stats();

    /* LRU demo */
    std::printf("\n  ── Demo: LRU Page Replacement ─────────────────────────────\n");
    mem_cleanup();
    mem_init();
    mem_set_replacement("LRU");
    mem_allocate(0, 16);

    for (int i = 0; i < 8; i++) {
        mem_access(0, static_cast<uint16_t>(i * PAGE_SIZE + 10));
    }
    /* Re-access page 0 to make it most recently used */
    mem_access(0, 0x0010);
    std::printf("\n  Page 0 re-accessed (now MRU). Accessing page 8 — LRU evicts page 1:\n");
    mem_access(0, static_cast<uint16_t>(8 * PAGE_SIZE + 10));

    mem_print_frames();
    mem_print_stats();
    std::printf("\n");
}

int main(void) {
    mem_init();

    std::printf("\n");
    std::printf("  ╔══════════════════════════════════════════════════════════╗\n");
    std::printf("  ║   Subsystem B: Memory Management & Virtual Memory      ║\n");
    std::printf("  ╚══════════════════════════════════════════════════════════╝\n");
    std::printf("\n  Type 'help' for commands.\n\n");

    char line[256];
    while (true) {
        std::printf("mem> ");
        std::fflush(stdout);
        if (std::fgets(line, sizeof(line), stdin) == nullptr) break;
        line[std::strcspn(line, "\n")] = '\0';

        char *trimmed = trim(line);
        if (std::strlen(trimmed) == 0) continue;

        char *cmd = std::strtok(trimmed, " \t");
        std::string command(cmd);

        if (command == "alloc") {
            char *pid_s = std::strtok(nullptr, " \t");
            char *pages_s = std::strtok(nullptr, " \t");
            if (!pid_s || !pages_s) {
                std::printf("  Usage: alloc <pid> <pages>\n"); continue;
            }
            int r = mem_allocate(std::atoi(pid_s), std::atoi(pages_s));
            if (r == 0) std::printf("  Allocated %s pages for PID %s\n", pages_s, pid_s);
            else std::printf("  Error (%d)\n", r);
        }
        else if (command == "access") {
            char *pid_s = std::strtok(nullptr, " \t");
            char *addr_s = std::strtok(nullptr, " \t");
            if (!pid_s || !addr_s) {
                std::printf("  Usage: access <pid> <hex_addr>\n"); continue;
            }
            uint16_t addr = static_cast<uint16_t>(std::strtol(addr_s, nullptr, 16));
            int r = mem_access(std::atoi(pid_s), addr);
            if (r >= 0) std::printf("  Logical 0x%04X -> Physical 0x%04X\n", addr, r);
            else std::printf("  Error (%d)\n", r);
        }
        else if (command == "free") {
            char *pid_s = std::strtok(nullptr, " \t");
            if (!pid_s) { std::printf("  Usage: free <pid>\n"); continue; }
            int r = mem_free(std::atoi(pid_s));
            if (r == 0) std::printf("  Freed PID %s\n", pid_s);
            else std::printf("  Error (%d)\n", r);
        }
        else if (command == "set_algo") {
            char *algo = std::strtok(nullptr, " \t");
            if (!algo) { std::printf("  Usage: set_algo <FIFO|LRU>\n"); continue; }
            int r = mem_set_replacement(algo);
            if (r == 0) std::printf("  Algorithm set to %s\n", algo);
            else std::printf("  Unknown: %s\n", algo);
        }
        else if (command == "page_table") {
            char *pid_s = std::strtok(nullptr, " \t");
            if (!pid_s) { std::printf("  Usage: page_table <pid>\n"); continue; }
            mem_print_page_table(std::atoi(pid_s));
        }
        else if (command == "frames") { mem_print_frames(); }
        else if (command == "stats") { mem_print_stats(); }
        else if (command == "demo") { run_demo(); }
        else if (command == "help") { print_help(); }
        else if (command == "exit" || command == "quit") { break; }
        else { std::printf("  Unknown command. Type 'help'.\n"); }
    }

    mem_cleanup();
    std::printf("  Goodbye!\n");
    return 0;
}

/*
 * main.cpp - MOSS Unified Command-Line Interface & System Logger
 * Mini Operating System Services Simulator
 *
 * Integrates all three subsystems (Scheduling, Memory, Synchronization)
 * into a single interactive command-line simulator with logging.
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
#include "moss_mem.h"
#include "moss_sync.h"

/* ============================================================
 * Logging Implementation
 * ============================================================ */

static const char* log_level_str(LogLevel level) {
    switch (level) {
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "????";
    }
}

void moss_log(LogLevel level, const char *fmt, ...) {
    /* Get current time */
    std::time_t now = std::time(nullptr);
    struct tm *tm_info = std::localtime(&now);
    char time_buf[20];
    std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);

    /* Print log prefix */
    std::printf("  [%s] [%s] ", time_buf, log_level_str(level));

    /* Print formatted message */
    va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    va_end(args);

    std::printf("\n");
}

/* ============================================================
 * Command Parsing Helpers
 * ============================================================ */

const int MAX_CMD_LEN = 256;
const int MAX_ARGS = 10;

/* Trim leading/trailing whitespace */
static char* trim(char *str) {
    while (std::isspace(static_cast<unsigned char>(*str))) str++;
    if (*str == 0) return str;
    char *end = str + std::strlen(str) - 1;
    while (end > str && std::isspace(static_cast<unsigned char>(*end))) end--;
    *(end + 1) = '\0';
    return str;
}

/* Parse command line into command and arguments */
static int parse_command(char *line, char **cmd, char *args[], int max_args) {
    char *trimmed = trim(line);
    if (std::strlen(trimmed) == 0) {
        *cmd = nullptr;
        return 0;
    }

    *cmd = std::strtok(trimmed, " \t");
    if (*cmd == nullptr) return 0;

    int argc = 0;
    char *token;
    while ((token = std::strtok(nullptr, " \t")) != nullptr && argc < max_args) {
        args[argc++] = token;
    }

    return argc;
}

/* ============================================================
 * Help Text
 * ============================================================ */

static void print_help(void) {
    std::printf("\n");
    std::printf("  +--------------------------------------------------------------+\n");
    std::printf("  |           MOSS - Mini OS Services Simulator                 |\n");
    std::printf("  |                   Command Reference                         |\n");
    std::printf("  +--------------------------------------------------------------+\n");
    std::printf("\n");
    std::printf("  -- Process Management & Scheduling --------------------------\n");
    std::printf("  create_process <name> <burst> <arrival> [priority]  Create a process\n");
    std::printf("  list_processes                                      List all processes\n");
    std::printf("  terminate <pid>                                     Terminate a process\n");
    std::printf("  schedule <FCFS|RR> [quantum]                        Run scheduling\n");
    std::printf("  gantt                                               Show Gantt chart\n");
    std::printf("  stats                                               Show scheduling stats\n");
    std::printf("\n");
    std::printf("  -- Memory Management ----------------------------------------\n");
    std::printf("  alloc_memory <pid> <pages>                          Allocate memory\n");
    std::printf("  access_memory <pid> <hex_addr>                      Access memory address\n");
    std::printf("  free_memory <pid>                                   Free process memory\n");
    std::printf("  set_replacement <FIFO|LRU>                          Set replacement algo\n");
    std::printf("  page_table <pid>                                    Show page table\n");
    std::printf("  frames                                              Show frame state\n");
    std::printf("  mem_stats                                           Show memory stats\n");
    std::printf("\n");
    std::printf("  -- Synchronization & Protection -----------------------------\n");
    std::printf("  create_mutex <name>                                 Create a mutex\n");
    std::printf("  lock <resource_id> <pid>                            Lock a mutex\n");
    std::printf("  unlock <resource_id> <pid>                          Unlock a mutex\n");
    std::printf("  create_sem <name> <value>                           Create a semaphore\n");
    std::printf("  sem_wait <sem_id> <pid>                             Semaphore wait (P)\n");
    std::printf("  sem_signal <sem_id> <pid>                           Semaphore signal (V)\n");
    std::printf("  run_pc <buffer_size> <num_items>                    Producer-Consumer sim\n");
    std::printf("  run_rw <readers> <writers> <ops>                     Readers-Writers sim\n");
    std::printf("  check_perm <pid> <resource> <action>                Check permission\n");
    std::printf("  set_role <pid> <admin|user>                         Set process role\n");
    std::printf("  detect_deadlock                                     Run deadlock detection\n");
    std::printf("  sync_state                                          Show sync state\n");
    std::printf("\n");
    std::printf("  -- System ---------------------------------------------------\n");
    std::printf("  demo_vertical                                       Run vertical slice demo\n");
    std::printf("  help                                                Show this help\n");
    std::printf("  exit                                                Exit simulator\n");
    std::printf("\n");
}

/* ============================================================
 * Vertical Slice Demo
 * ============================================================ */

static void run_vertical_slice_demo(void) {
    std::printf("\n");
    std::printf("  +--------------------------------------------------------------+\n");
    std::printf("  |              VERTICAL SLICE DEMONSTRATION                   |\n");
    std::printf("  |     End-to-end flow through all MOSS subsystems             |\n");
    std::printf("  +--------------------------------------------------------------+\n");
    std::printf("\n");

    /* Step 1: Process creation */
    std::printf("  -- Step 1: Process Creation (Subsystem A) -------------------\n");
    int pid1 = sched_create_process("WebServer", 5, 0, 1, ROLE_ADMIN);
    int pid2 = sched_create_process("Database", 3, 1, 2, ROLE_USER);
    int pid3 = sched_create_process("Logger", 2, 2, 3, ROLE_USER);
    std::printf("\n");

    /* Step 2: CPU Scheduling */
    std::printf("  -- Step 2: CPU Scheduling - Round Robin (Subsystem A) -------\n");
    sched_run_rr(2);
    sched_print_gantt();
    sched_print_stats();
    std::printf("\n");

    /* Re-create processes for the rest of the demo (scheduling terminated them) */
    sched_cleanup();
    sched_init();
    pid1 = sched_create_process("WebServer", 5, 0, 1, ROLE_ADMIN);
    pid2 = sched_create_process("Database", 3, 1, 2, ROLE_USER);
    pid3 = sched_create_process("Logger", 2, 2, 3, ROLE_USER);

    /* Step 3: Memory allocation and access */
    std::printf("  -- Step 3: Memory Management (Subsystem B) ------------------\n");
    mem_allocate(pid1, 4);  /* 4 pages for WebServer */
    mem_allocate(pid2, 3);  /* 3 pages for Database */

    std::printf("\n  Accessing memory addresses:\n");
    mem_access(pid1, 0x0010);  /* Page 0 */
    mem_access(pid1, 0x0120);  /* Page 1 */
    mem_access(pid2, 0x0050);  /* Page 0 */
    mem_access(pid1, 0x0210);  /* Page 2 */
    mem_access(pid2, 0x0150);  /* Page 1 */

    std::printf("\n");
    mem_print_frames();
    mem_print_stats();
    std::printf("\n");

    /* Step 4: Synchronization */
    std::printf("  -- Step 4: Synchronization (Subsystem C) --------------------\n");
    int mutex_id = sync_mutex_create("db_lock");
    sync_mutex_lock(mutex_id, pid1);
    sync_mutex_lock(mutex_id, pid2);  /* Should be blocked/queued */
    sync_mutex_unlock(mutex_id, pid1); /* Should hand off to pid2 */
    sync_mutex_unlock(mutex_id, pid2);
    std::printf("\n");

    /* Step 5: Access control - Permission check */
    std::printf("  -- Step 5: Access Control (Subsystem C) ---------------------\n");
    std::printf("  Checking permissions:\n");
    sync_check_permission(pid1, "memory", "execute");   /* ADMIN: allowed */
    sync_check_permission(pid2, "memory", "execute");   /* USER: denied */
    sync_check_permission(pid2, "memory", "read");      /* USER: allowed */
    sync_check_permission(pid3, "process", "write");    /* USER: denied */
    std::printf("\n");

    /* Step 6: Error scenario - Invalid memory access */
    std::printf("  -- Step 6: Error Scenario - Invalid Memory Access -----------\n");
    int result = mem_access(pid1, 0x0F00); /* Out of range for 4 pages */
    if (result < 0) {
        std::printf("  Error handled correctly: invalid memory access returned code %d\n", result);
    }
    std::printf("\n");

    /* Step 7: Deadlock detection */
    std::printf("  -- Step 7: Deadlock Detection -------------------------------\n");
    int m1 = sync_mutex_create("res_A");
    int m2 = sync_mutex_create("res_B");
    sync_mutex_lock(m1, pid1);  /* P1 holds res_A */
    sync_mutex_lock(m2, pid2);  /* P2 holds res_B */
    sync_mutex_lock(m2, pid1);  /* P1 waits for res_B (held by P2) */
    sync_mutex_lock(m1, pid2);  /* P2 waits for res_A (held by P1) -> DEADLOCK */
    std::printf("\n  Running deadlock detection...\n");
    sync_detect_deadlock();
    std::printf("\n");

    std::printf("  +--------------------------------------------------------------+\n");
    std::printf("  |              VERTICAL SLICE COMPLETE                        |\n");
    std::printf("  |  All subsystems demonstrated with cross-subsystem           |\n");
    std::printf("  |  interaction and error handling.                            |\n");
    std::printf("  +--------------------------------------------------------------+\n\n");
}

/* ============================================================
 * Command Handlers
 * ============================================================ */

static void handle_create_process(int argc, char *args[]) {
    if (argc < 3) {
        std::printf("  Usage: create_process <name> <burst> <arrival> [priority]\n");
        return;
    }

    const char *name = args[0];
    int burst = std::atoi(args[1]);
    int arrival = std::atoi(args[2]);
    int priority = (argc >= 4) ? std::atoi(args[3]) : 0;

    int pid = sched_create_process(name, burst, arrival, priority, ROLE_USER);
    if (pid >= 0) {
        std::printf("  Process created with PID %d\n", pid);
    } else {
        std::printf("  Error: could not create process (code %d)\n", pid);
    }
}

static void handle_terminate(int argc, char *args[]) {
    if (argc < 1) {
        std::printf("  Usage: terminate <pid>\n");
        return;
    }

    int pid = std::atoi(args[0]);
    int result = sched_terminate_process(pid);
    if (result == MOSS_SUCCESS) {
        std::printf("  Process PID %d terminated\n", pid);
        /* Also free its memory */
        mem_free(pid);
    } else {
        std::printf("  Error: process PID %d not found\n", pid);
    }
}

static void handle_schedule(int argc, char *args[]) {
    if (argc < 1) {
        std::printf("  Usage: schedule <FCFS|RR> [quantum]\n");
        return;
    }

    int result;
    std::string algo(args[0]);

    if (algo == "FCFS") {
        result = sched_run_fcfs();
    } else if (algo == "RR") {
        int quantum = (argc >= 2) ? std::atoi(args[1]) : 2;
        result = sched_run_rr(quantum);
    } else {
        std::printf("  Unknown algorithm: %s (use FCFS or RR)\n", args[0]);
        return;
    }

    if (result == MOSS_SUCCESS) {
        std::printf("  Scheduling completed. Use 'gantt' and 'stats' to view results.\n");
    } else {
        std::printf("  Scheduling failed (code %d). Make sure processes exist.\n", result);
    }
}

static void handle_alloc_memory(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: alloc_memory <pid> <pages>\n");
        return;
    }

    int pid = std::atoi(args[0]);
    int pages = std::atoi(args[1]);
    int result = mem_allocate(pid, pages);
    if (result == MOSS_SUCCESS) {
        std::printf("  Allocated %d pages for PID %d\n", pages, pid);
    } else {
        std::printf("  Error: allocation failed (code %d)\n", result);
    }
}

static void handle_access_memory(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: access_memory <pid> <hex_addr>\n");
        return;
    }

    int pid = std::atoi(args[0]);
    uint16_t addr = static_cast<uint16_t>(std::strtol(args[1], nullptr, 16));

    int result = mem_access(pid, addr);
    if (result >= 0) {
        std::printf("  Logical 0x%04X -> Physical 0x%04X\n", addr, result);
    } else {
        std::printf("  Memory access failed (code %d)\n", result);
    }
}

static void handle_free_memory(int argc, char *args[]) {
    if (argc < 1) {
        std::printf("  Usage: free_memory <pid>\n");
        return;
    }

    int pid = std::atoi(args[0]);
    int result = mem_free(pid);
    if (result == MOSS_SUCCESS) {
        std::printf("  Memory freed for PID %d\n", pid);
    } else {
        std::printf("  Error: no memory allocated for PID %d\n", pid);
    }
}

static void handle_set_replacement(int argc, char *args[]) {
    if (argc < 1) {
        std::printf("  Usage: set_replacement <FIFO|LRU>\n");
        return;
    }

    int result = mem_set_replacement(args[0]);
    if (result == MOSS_SUCCESS) {
        std::printf("  Replacement algorithm set to %s\n", args[0]);
    } else {
        std::printf("  Unknown algorithm: %s (use FIFO or LRU)\n", args[0]);
    }
}

static void handle_page_table(int argc, char *args[]) {
    if (argc < 1) {
        std::printf("  Usage: page_table <pid>\n");
        return;
    }
    mem_print_page_table(std::atoi(args[0]));
}

static void handle_create_mutex(int argc, char *args[]) {
    if (argc < 1) {
        std::printf("  Usage: create_mutex <name>\n");
        return;
    }

    int id = sync_mutex_create(args[0]);
    if (id >= 0) {
        std::printf("  Mutex created with ID %d\n", id);
    } else {
        std::printf("  Error: could not create mutex (code %d)\n", id);
    }
}

static void handle_lock(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: lock <resource_id> <pid>\n");
        return;
    }

    int rid = std::atoi(args[0]);
    int pid = std::atoi(args[1]);
    int result = sync_mutex_lock(rid, pid);
    if (result == MOSS_SUCCESS) {
        std::printf("  Mutex %d locked by PID %d\n", rid, pid);
    } else if (result == MOSS_ERR_BUSY) {
        std::printf("  Mutex %d is busy. PID %d added to wait queue.\n", rid, pid);
    } else {
        std::printf("  Error: lock failed (code %d)\n", result);
    }
}

static void handle_unlock(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: unlock <resource_id> <pid>\n");
        return;
    }

    int rid = std::atoi(args[0]);
    int pid = std::atoi(args[1]);
    int result = sync_mutex_unlock(rid, pid);
    if (result == MOSS_SUCCESS) {
        std::printf("  Mutex %d unlocked by PID %d\n", rid, pid);
    } else if (result == MOSS_ERR_DENIED) {
        std::printf("  Error: PID %d does not own mutex %d\n", pid, rid);
    } else {
        std::printf("  Error: unlock failed (code %d)\n", result);
    }
}

static void handle_create_sem(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: create_sem <name> <value>\n");
        return;
    }

    int id = sync_sem_create(args[0], std::atoi(args[1]));
    if (id >= 0) {
        std::printf("  Semaphore created with ID %d\n", id);
    } else {
        std::printf("  Error: could not create semaphore (code %d)\n", id);
    }
}

static void handle_sem_wait(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: sem_wait <sem_id> <pid>\n");
        return;
    }

    int sid = std::atoi(args[0]);
    int pid = std::atoi(args[1]);
    int result = sync_sem_wait(sid, pid);
    if (result == MOSS_SUCCESS) {
        std::printf("  Semaphore %d: wait successful for PID %d\n", sid, pid);
    } else if (result == MOSS_ERR_BUSY) {
        std::printf("  Semaphore %d: PID %d blocked (value=0)\n", sid, pid);
    } else {
        std::printf("  Error: sem_wait failed (code %d)\n", result);
    }
}

static void handle_sem_signal(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: sem_signal <sem_id> <pid>\n");
        return;
    }

    int sid = std::atoi(args[0]);
    int pid = std::atoi(args[1]);
    int result = sync_sem_signal(sid, pid);
    if (result == MOSS_SUCCESS) {
        std::printf("  Semaphore %d: signal by PID %d\n", sid, pid);
    } else {
        std::printf("  Error: sem_signal failed (code %d)\n", result);
    }
}

static void handle_run_pc(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: run_pc <buffer_size> <num_items>\n");
        return;
    }

    int buf_size = std::atoi(args[0]);
    int items = std::atoi(args[1]);
    int result = sync_run_producer_consumer(buf_size, items);
    if (result != MOSS_SUCCESS) {
        std::printf("  Error: Producer-Consumer simulation failed (code %d)\n", result);
    }
}

static void handle_check_perm(int argc, char *args[]) {
    if (argc < 3) {
        std::printf("  Usage: check_perm <pid> <resource> <action>\n");
        std::printf("  Resources: memory, process, sync_resource\n");
        std::printf("  Actions: read, write, execute, admin\n");
        return;
    }

    int pid = std::atoi(args[0]);
    int result = sync_check_permission(pid, args[1], args[2]);
    if (result == MOSS_SUCCESS) {
        std::printf("  Permission GRANTED: PID %d -> %s:%s\n", pid, args[1], args[2]);
    } else if (result == MOSS_ERR_DENIED) {
        std::printf("  Permission DENIED: PID %d -> %s:%s\n", pid, args[1], args[2]);
    } else {
        std::printf("  Error: permission check failed (code %d)\n", result);
    }
}

static void handle_set_role(int argc, char *args[]) {
    if (argc < 2) {
        std::printf("  Usage: set_role <pid> <admin|user>\n");
        return;
    }

    int pid = std::atoi(args[0]);
    std::string role_name(args[1]);
    UserRole role;

    if (role_name == "admin") {
        role = ROLE_ADMIN;
    } else if (role_name == "user") {
        role = ROLE_USER;
    } else {
        std::printf("  Unknown role: %s (use admin or user)\n", args[1]);
        return;
    }

    int result = sync_set_role(pid, role);
    if (result == MOSS_SUCCESS) {
        std::printf("  Role set: PID %d -> %s\n", pid, args[1]);
    } else {
        std::printf("  Error: process PID %d not found\n", pid);
    }
}

/* ============================================================
 * Banner
 * ============================================================ */

static void print_banner(void) {
    std::printf("\n");
    std::printf("  +--------------------------------------------------------------+\n");
    std::printf("  |                                                            |\n");
    std::printf("  |   # # #   # # #  # # # # #  # # # # # # #  # # # # # # #   |\n");
    std::printf("  |   # # # # # # # # # # # # # # # # # # # #  # # # # # # #   |\n");
    std::printf("  |   # # # # # # # # # # # # # # # # # # # #  # # # # # # #   |\n");
    std::printf("  |   # # # # # # # # # # # # # # # # # # # #  # # # # # # #   |\n");
    std::printf("  |   # # # # # # # # # # # # # # # # # # # #  # # # # # # #   |\n");
    std::printf("  |   # # # # # # # # # # # # # # # # # # # #  # # # # # # #   |\n");
    std::printf("  |                                                            |\n");
    std::printf("  |   Mini Operating System Services Simulator                 |\n");
    std::printf("  |   COSC 414 - Operating Systems                             |\n");
    std::printf("  |                                                            |\n");
    std::printf("  +--------------------------------------------------------------+\n");
    std::printf("\n");
    std::printf("  Type 'help' for a list of commands.\n\n");
}

/* ============================================================
 * Main Loop
 * ============================================================ */

int main(void) {
    /* Initialize all subsystems */
    sched_init();
    mem_init();
    sync_init();

    print_banner();

    char line[MAX_CMD_LEN];
    char *cmd;
    char *args[MAX_ARGS];

    while (true) {
        std::printf("moss> ");
        std::fflush(stdout);

        if (std::fgets(line, sizeof(line), stdin) == nullptr) {
            std::printf("\n");
            break;
        }

        /* Remove newline */
        line[std::strcspn(line, "\n")] = '\0';

        int argc = parse_command(line, &cmd, args, MAX_ARGS);
        if (cmd == nullptr) continue;

        std::string command(cmd);

        /* -- Process Management -- */
        if (command == "create_process") {
            handle_create_process(argc, args);
        }
        else if (command == "list_processes") {
            sched_list_processes();
        }
        else if (command == "terminate") {
            handle_terminate(argc, args);
        }
        else if (command == "schedule") {
            handle_schedule(argc, args);
        }
        else if (command == "gantt") {
            sched_print_gantt();
        }
        else if (command == "stats") {
            sched_print_stats();
        }
        /* -- Memory Management -- */
        else if (command == "alloc_memory") {
            handle_alloc_memory(argc, args);
        }
        else if (command == "access_memory") {
            handle_access_memory(argc, args);
        }
        else if (command == "free_memory") {
            handle_free_memory(argc, args);
        }
        else if (command == "set_replacement") {
            handle_set_replacement(argc, args);
        }
        else if (command == "page_table") {
            handle_page_table(argc, args);
        }
        else if (command == "frames") {
            mem_print_frames();
        }
        else if (command == "mem_stats") {
            mem_print_stats();
        }
        /* -- Synchronization & Protection -- */
        else if (command == "create_mutex") {
            handle_create_mutex(argc, args);
        }
        else if (command == "lock") {
            handle_lock(argc, args);
        }
        else if (command == "unlock") {
            handle_unlock(argc, args);
        }
        else if (command == "create_sem") {
            handle_create_sem(argc, args);
        }
        else if (command == "sem_wait") {
            handle_sem_wait(argc, args);
        }
        else if (command == "sem_signal") {
            handle_sem_signal(argc, args);
        }
        else if (command == "run_pc") {
            handle_run_pc(argc, args);
        }
        else if (command == "run_rw") {
            if (argc < 3) {
                std::printf("  Usage: run_rw <readers> <writers> <ops>\n");
            } else {
                int r = sync_run_readers_writers(std::atoi(args[0]), std::atoi(args[1]), std::atoi(args[2]));
                if (r != MOSS_SUCCESS) {
                    std::printf("  Error: Readers-Writers simulation failed (code %d)\n", r);
                }
            }
        }
        else if (command == "check_perm") {
            handle_check_perm(argc, args);
        }
        else if (command == "set_role") {
            handle_set_role(argc, args);
        }
        else if (command == "detect_deadlock") {
            sync_detect_deadlock();
        }
        else if (command == "sync_state") {
            sync_print_state();
        }
        /* -- System -- */
        else if (command == "demo_vertical") {
            /* Reset everything for a clean demo */
            sched_cleanup();
            mem_cleanup();
            sync_cleanup();
            sched_init();
            mem_init();
            sync_init();
            run_vertical_slice_demo();
        }
        else if (command == "help") {
            print_help();
        }
        else if (command == "exit" || command == "quit") {
            std::printf("  Shutting down MOSS...\n");
            break;
        }
        else {
            std::printf("  Unknown command: '%s'. Type 'help' for a list of commands.\n", cmd);
        }
    }

    /* Cleanup */
    sched_cleanup();
    mem_cleanup();
    sync_cleanup();

    std::printf("  MOSS terminated. Goodbye!\n");
    return 0;
}

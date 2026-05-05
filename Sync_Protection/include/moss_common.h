/*
 * common.h - Shared types, constants, and error codes for MOSS
 * Mini Operating System Services Simulator
 *
 * This header defines all shared data structures and constants
 * used across subsystems A (Scheduling), B (Memory), and C (Sync).
 */

#ifndef MOSS_COMMON_H
#define MOSS_COMMON_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <string>

/* ============================================================
 * Error Codes
 * All API functions return 0 on success, negative on error.
 * ============================================================ */
#define MOSS_SUCCESS       0
#define MOSS_ERR_NOT_FOUND -1
#define MOSS_ERR_FULL      -2
#define MOSS_ERR_INVALID   -3
#define MOSS_ERR_DENIED    -4
#define MOSS_ERR_DEADLOCK  -5
#define MOSS_ERR_BUSY      -6
#define MOSS_ERR_NO_MEM    -7

/* ============================================================
 * System Limits
 * ============================================================ */
const int MAX_PROCESSES    = 32;
const int MAX_RESOURCES    = 16;
const int MAX_SEMAPHORES   = 16;
const int MAX_FRAMES       = 8;
const int MAX_PAGES        = 64;    /* Max pages per process */
const int PAGE_SIZE        = 256;   /* Bytes per page */
const int LOGICAL_ADDR_BITS = 16;   /* 16-bit logical address space */
const int MAX_NAME_LEN     = 32;
const int MAX_GANTT        = 256;   /* Max Gantt chart entries */
const int MAX_BUFFER_SIZE  = 16;    /* Max Producer-Consumer buffer */
const int MAX_WAIT_QUEUE   = 32;

/* ============================================================
 * Log Levels
 * ============================================================ */
enum LogLevel {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

/* ============================================================
 * Process States
 * ============================================================ */
enum ProcessState {
    PROC_NEW,
    PROC_READY,
    PROC_RUNNING,
    PROC_WAITING,
    PROC_TERMINATED
};

/* ============================================================
 * User Roles (for access control)
 * ============================================================ */
enum UserRole {
    ROLE_ADMIN,
    ROLE_USER
};

/* ============================================================
 * Process Control Block (PCB)
 * ============================================================ */
struct PCB {
    int pid;
    char name[MAX_NAME_LEN];
    ProcessState state;
    int priority;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int waiting_time;
    int turnaround_time;
    int completion_time;
    UserRole role;
    int active;  /* 1 = in use, 0 = free slot */
};

/* ============================================================
 * Gantt Chart Entry
 * ============================================================ */
struct GanttEntry {
    int pid;
    int start_time;
    int end_time;
};

/* ============================================================
 * Logging Function (implemented in main.cpp)
 * ============================================================ */
void moss_log(LogLevel level, const char *fmt, ...);

/* ============================================================
 * Helper: State to string
 * ============================================================ */
inline const char* proc_state_str(ProcessState s) {
    switch (s) {
        case PROC_NEW:        return "NEW";
        case PROC_READY:      return "READY";
        case PROC_RUNNING:    return "RUNNING";
        case PROC_WAITING:    return "WAITING";
        case PROC_TERMINATED: return "TERMINATED";
        default:              return "UNKNOWN";
    }
}

inline const char* role_str(UserRole r) {
    switch (r) {
        case ROLE_ADMIN: return "ADMIN";
        case ROLE_USER:  return "USER";
        default:         return "UNKNOWN";
    }
}

#endif /* MOSS_COMMON_H */

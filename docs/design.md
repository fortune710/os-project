# MOSS System Design Document

## 1. Introduction

MOSS (Mini Operating System Services Simulator) is a user-space simulator that models core operating system services including process management, CPU scheduling, memory management with virtual memory, synchronization primitives, and role-based access control.

The simulator runs on Ubuntu Linux 22.04 LTS and is implemented in C using only standard POSIX libraries. It provides an interactive command-line interface for experimenting with OS policies and observing their effects.

---

## 2. System Architecture

### 2.1 High-Level Design

MOSS follows a modular architecture with three independent subsystems connected through a central integration layer:

```
┌─────────────────────────────────────────────────┐
│                main.c                           │
│        (CLI, Logging, Integration)              │
├────────────┬──────────────┬─────────────────────┤
│  Subsystem A  │  Subsystem B  │  Subsystem C       │
│  Scheduling   │  Memory Mgmt  │  Sync & Protection │
│  (sched.c)    │  (mem.c)      │  (sync.c)          │
├────────────┴──────────────┴─────────────────────┤
│                common.h                         │
│        (Shared Types & Constants)               │
└─────────────────────────────────────────────────┘
```

### 2.2 Design Principles

1. **Modularity**: Each subsystem is self-contained with a well-defined API
2. **Encapsulation**: Internal state is private to each subsystem; access only through API calls
3. **Consistent Error Handling**: All functions return status codes; no direct printing in core APIs
4. **Clean Interfaces**: Functions prefixed by subsystem (`sched_`, `mem_`, `sync_`)
5. **Separation of Concerns**: CLI/logging handled by main.c; subsystems focus on logic

---

## 3. Subsystem A: Process Management & CPU Scheduling

### 3.1 Data Structures

**Process Control Block (PCB)**:
```c
typedef struct {
    int pid;              // Unique process identifier
    char name[32];        // Human-readable process name
    ProcessState state;   // NEW, READY, RUNNING, WAITING, TERMINATED
    int priority;         // Priority (lower = higher priority)
    int arrival_time;     // When the process enters the system
    int burst_time;       // Total CPU time required
    int remaining_time;   // CPU time still needed
    int waiting_time;     // Time spent waiting in ready queue
    int turnaround_time;  // Total time from arrival to completion
    int completion_time;  // Time when process finished
    UserRole role;        // ADMIN or USER (for access control)
    int active;           // 1 if slot is in use
} PCB;
```

**Process Table**: Fixed-size array of 32 PCB slots.

**Gantt Chart**: Array of `(pid, start_time, end_time)` tuples recording the scheduling timeline.

### 3.2 Scheduling Algorithms

#### FCFS (First-Come, First-Served)
- **Type**: Non-preemptive
- **Queue Discipline**: Processes sorted by arrival time; ties broken by PID
- **Idle Handling**: If no process has arrived, time advances to next arrival
- **Complexity**: O(n log n) for sort + O(n) for execution

#### Round Robin
- **Type**: Preemptive
- **Mechanism**: Each process runs for at most `quantum` time units, then is re-enqueued
- **Queue**: Processes enqueued in arrival order; re-enqueued at back after preemption
- **New Arrivals**: Enqueued before the preempted process (checked during each quantum)
- **Completion**: Process removed from queue when `remaining_time` reaches 0
- **Complexity**: O(n × total_burst / quantum)

### 3.3 Statistics

Calculated after each scheduling run:
- **Per-process**: Waiting time = Turnaround - Burst, Turnaround = Completion - Arrival
- **Aggregate**: Average waiting time, Average turnaround time

### 3.4 Edge Cases Handled
- Empty ready queue → returns `MOSS_ERR_INVALID`
- Simultaneous arrivals → ordered by PID
- Idle gaps between process arrivals → recorded in Gantt chart as `pid = -1`
- Zero or negative quantum → returns `MOSS_ERR_INVALID`

---

## 4. Subsystem B: Memory Management & Virtual Memory

### 4.1 Configuration

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Page Size | 256 bytes | Simple, demonstrates concepts clearly |
| Logical Address Width | 16 bits | 64KB address space, 256 pages max |
| Physical Frames | 8 | Small enough to demonstrate replacement |
| Max Pages per Process | 64 | Sufficient for demonstration |

### 4.2 Data Structures

**Page Table Entry**:
```c
typedef struct {
    int frame_number;   // Physical frame, -1 if not loaded
    int valid;          // 1 if page is in a physical frame
} PageTableEntry;
```

**Physical Frame**:
```c
typedef struct {
    int occupied;       // 1 if frame is in use
    int pid;            // Owning process
    int page_number;    // Which page of the process
    int load_order;     // For FIFO: monotonic load timestamp
    int last_access;    // For LRU: monotonic access timestamp
} Frame;
```

### 4.3 Address Translation

```
Logical Address (16-bit): |  Page Number  |  Offset  |
                          |   (8 bits)    | (8 bits) |

page_number  = logical_addr / 256
offset       = logical_addr % 256
physical_addr = frame_number * 256 + offset
```

### 4.4 Demand Paging

Pages are **not loaded** when memory is allocated. They are loaded on first access (demand paging). If the page is not in a frame (valid = 0), a **page fault** occurs and the page is loaded into a free frame (or evicts a victim if all frames are full).

### 4.5 Page Replacement Algorithms

#### FIFO (First-In, First-Out)
- **Eviction**: Frame with the smallest `load_order` is evicted
- **Tracking**: Monotonic counter incremented on each load
- **Anomaly**: Subject to Belady's anomaly

#### LRU (Least Recently Used)
- **Eviction**: Frame with the smallest `last_access` is evicted
- **Tracking**: Monotonic counter incremented on each access
- **Performance**: Generally better than FIFO, but requires tracking access times

### 4.6 Statistics Tracked
- Total memory accesses
- Total page faults
- Page hits (accesses - faults)
- Hit rate (percentage)

---

## 5. Subsystem C: Synchronization & Protection

### 5.1 Mutex Locks

- **Create**: Allocate a named mutex in an unlocked state
- **Lock**: If free, acquire immediately; if held, add to FIFO wait queue
- **Unlock**: Only owner can unlock; if waiters exist, hand off to next in queue
- **Re-entrant Protection**: Attempting to lock a mutex you already hold returns `MOSS_ERR_BUSY`

### 5.2 Semaphores

- **Create**: Initialize with a non-negative integer value
- **Wait (P)**: If value > 0, decrement; if value = 0, block (add to wait queue)
- **Signal (V)**: If waiters exist, wake first waiter; otherwise increment value
- **Counting**: Supports values > 1 for counting semaphore behavior

### 5.3 Producer-Consumer Simulation

Implements the classic bounded-buffer Producer-Consumer problem:

```
Semaphore empty = buffer_size   // tracks empty slots
Semaphore full = 0              // tracks filled slots
Mutex buffer_lock               // protects buffer access

Producer:                       Consumer:
  wait(empty)                     wait(full)
  lock(buffer_lock)               lock(buffer_lock)
  produce(item)                   consume(item)
  unlock(buffer_lock)             unlock(buffer_lock)
  signal(full)                    signal(empty)
```

The simulation alternates between producer and consumer actions, printing a step-by-step trace showing semaphore values, mutex state, and buffer occupancy.

### 5.4 Access Control Model

Role-based access control with two roles and four action types:

- **Roles**: `ADMIN` (full access), `USER` (limited access)
- **Resources**: `memory`, `process`, `sync_resource`
- **Actions**: `read`, `write`, `execute`, `admin`

General policy:
- Admins have unrestricted access to all resources
- Users can read all resources, execute processes and sync resources, but cannot write to processes or sync resources, and cannot execute memory or perform admin actions

### 5.5 Deadlock Detection

Uses **wait-for graph** analysis:
1. Build edges from mutex state: if process A holds mutex M and process B waits for M, add edge B → A
2. Detect cycles using **Floyd's cycle detection** (tortoise and hare)
3. If a cycle is found, report the deadlocked processes

---

## 6. Integration Architecture

### 6.1 Subsystem Interactions

| Interaction | Description |
|-------------|-------------|
| Sched → Mem | Memory allocated for created processes; freed on termination |
| Sched → Sync | Process roles used for access control checks |
| Mem → Sync | Permission checks before memory operations (optional) |
| Sync → Sched | `sched_get_process()` used to look up process roles |

### 6.2 Logging System

All operations are logged with:
- **Timestamp**: `HH:MM:SS` format
- **Severity**: `[INFO]`, `[WARN]`, `[ERROR]`
- **Message**: Descriptive text from the subsystem

Logging is implemented in `main.c` via the `moss_log()` function, which all subsystems call through the function declaration in `common.h`.

### 6.3 Error Handling Strategy

- All API functions return status codes (never exit or abort)
- Errors propagate upward to the CLI layer
- The CLI translates error codes into user-friendly messages
- At least one error scenario demonstrated: deadlock detection, invalid memory access, permission violation

---

## 7. Design Decisions & Trade-offs

### 7.1 Static vs Dynamic Allocation
**Decision**: Use fixed-size arrays (static allocation) for process table, frames, mutexes, and semaphores.

**Rationale**: Simplifies memory management, avoids dynamic allocation bugs, and is sufficient for a simulator. Trade-off: limited to compile-time-defined maximums.

### 7.2 Simulated vs Real Concurrency
**Decision**: All concurrency is logically simulated — no real threads.

**Rationale**: Project specification states "logical simulation of concurrency is sufficient." This avoids race conditions in the simulator itself while faithfully modeling OS behavior.

### 7.3 Demand Paging
**Decision**: Pages are loaded on first access, not at allocation time.

**Rationale**: More realistic modeling of actual virtual memory systems. The first access to any page triggers a page fault.

### 7.4 Gantt Chart as Post-Processing
**Decision**: The scheduler runs the entire schedule and records the Gantt chart, rather than stepping through time interactively.

**Rationale**: Simpler implementation and consistent output. The scheduling runs deterministically to completion.

---

## 8. Limitations & Future Work

### Current Limitations
- No preemptive priority scheduling (undergraduate scope)
- No multi-level feedback queue
- No disk I/O simulation
- Fixed access control matrix (not runtime-configurable)
- Deadlock detection but no deadlock recovery

### Potential Extensions (COSC 514)
- Priority scheduling with preemption
- Optimal or Working Set page replacement
- Readers-Writers synchronization problem
- Quantitative performance analysis
- Configurable access control policies

# MOSS — Mini Operating System Services Simulator
## Final Project Report

**Team:** Fortune Alebiosu · Samir Mahmoud · Caroline Njenga · Ahuine Okoeguale  
**Course:** COSC 414 — Operating Systems  
**Project:** Mini Operating System Services Simulator (MOSS)  
**Platform:** Ubuntu Linux 22.04 LTS  
**Language:** C++ (G++ 11+, C++17 Standard)  

---

## Table of Contents

1. Introduction
2. System Architecture
3. Subsystem A: Process Management & CPU Scheduling
4. Subsystem B: Memory Management & Virtual Memory
5. Subsystem C: Synchronization & Protection
6. System Integration
7. Testing & Verification
8. Design Decisions & Trade-offs
9. Known Limitations & Future Work
10. Conclusion

---

## 1. Introduction

Operating systems are among the most complex pieces of software ever engineered. They
manage hardware resources, enforce security boundaries, schedule competing processes, and
provide the illusion of infinite memory — all simultaneously and transparently. Understanding
these concepts in the abstract is one thing; building a working system that models them is
another entirely.

MOSS — the Mini Operating System Services Simulator — is a user-space application that
models three core pillars of operating system design: **CPU scheduling**, **virtual memory
management**, and **synchronization with access control**. Rather than modifying a real
kernel, MOSS runs entirely in user space on Ubuntu Linux and faithfully simulates how an
operating system would handle these concerns. Users interact with MOSS through a unified
command-line interface that accepts commands to create processes, schedule them, allocate
and access memory, manage locks and semaphores, and detect deadlocks.

The goals of this project are threefold. First, to apply core operating system concepts in a
practical, hands-on implementation. Second, to design modular software with clean, well-defined
interfaces between subsystems. Third, to reason about the trade-offs inherent in every OS
design decision — from the fairness of scheduling algorithms to the overhead of page
replacement strategies.

MOSS is implemented in approximately 2,400 lines of C++ across four source files, four
header files, and a comprehensive test suite of 78 automated tests. It compiles with zero
warnings under strict compiler flags and runs on standard Ubuntu Linux 22.04 LTS using
only POSIX-compatible standard libraries.

---

## 2. System Architecture

### 2.1 High-Level Design

MOSS follows a modular architecture organized around three independent subsystems connected
through a central integration layer:

```
┌─────────────────────────────────────────────────────────────────────┐
│                          main.cpp                                   │
│           CLI · Logging (moss_log) · Integration · Vertical Slice   │
└──────────────┬──────────────────────┬───────────────────┬───────────┘
    sched_*()  │          mem_*()     │       sync_*()    │
               ▼                      ▼                   ▼
  ┌────────────────────┐  ┌───────────────────┐  ┌────────────────────┐
  │    Subsystem A     │  │   Subsystem B     │  │    Subsystem C     │
  │    sched.cpp       │  │   mem.cpp         │  │    sync.cpp        │
  │                    │  │                   │  │                    │
  │  Process table     │  │  Page tables      │  │  Mutex array       │
  │  FCFS / RR         │  │  FIFO / LRU       │  │  Semaphores        │
  │  Gantt chart       │  │  Demand paging    │  │  Producer-Consumer │
  │  Statistics        │  │  Addr translation │  │  Readers-Writers   │
  └────────┬───────────┘  └───────────────────┘  └─────────┬──────────┘
           │◄──────────────────────────────────────────────┘
           │  sync_check_permission() / sync_set_role()
           │  call sched_get_process(pid) to read/write PCB role field
           │
┌─────────────────────────────────────────────────────────────────────┐
│                        moss_common.h                                │
│      Shared types: PCB · ProcessState · UserRole · GanttEntry       │
│      Constants: MAX_PROCESSES · PAGE_SIZE · MAX_FRAMES · etc.       │
│      Error codes: MOSS_SUCCESS · MOSS_ERR_* (7 distinct codes)      │
└─────────────────────────────────────────────────────────────────────┘
```

The integration layer in `main.cpp` serves three purposes: it provides the interactive
command-line interface through which users issue commands, it implements the system-wide
logging infrastructure that all subsystems use to report their activity, and it orchestrates
the vertical slice demonstration that exercises all subsystems in a single end-to-end flow.

### 2.2 Repository Structure

```
project-root/
├── README.md                    Project overview and usage guide
├── Makefile                     Build system (g++, C++17)
├── include/
│   ├── moss_common.h            Shared types, constants, error codes
│   ├── moss_sched.h             Scheduler public API declarations
│   ├── moss_mem.h               Memory management public API
│   └── moss_sync.h              Synchronization public API
├── src/
│   ├── main.cpp                 CLI, logging, integration, vertical slice
│   ├── sched/
│   │   └── sched.cpp            Process management & CPU scheduling
│   ├── mem/
│   │   └── mem.cpp              Paging & virtual memory
│   └── sync/
│       └── sync.cpp             Mutexes, semaphores, access control
├── tests/
│   └── basic_tests.cpp          78 automated tests
└── docs/
    ├── api.md                   Full API reference
    ├── design.md                Technical design document
    └── report.md                This report
```

### 2.3 Design Principles

Five key principles guided the architecture of MOSS:

**Modularity.** Each subsystem is self-contained in its own source file with a dedicated
header declaring its public API. Subsystems do not share internal data structures or global
variables. All communication between subsystems occurs through documented function calls.

**Encapsulation.** Internal state — process tables, page tables, mutex structures — is
declared `static` within each source file, making it invisible to other translation units.
Other subsystems and the integration layer can only observe or modify this state through
the public API functions.

**Consistent error handling.** Every API function returns an integer status code: zero
(`MOSS_SUCCESS`) for success, and a specific negative value for each type of error
(`MOSS_ERR_NOT_FOUND`, `MOSS_ERR_FULL`, `MOSS_ERR_INVALID`, `MOSS_ERR_DENIED`,
`MOSS_ERR_DEADLOCK`, `MOSS_ERR_BUSY`). Errors propagate upward to the CLI layer, which
translates them into user-friendly messages.

**No printing in core APIs.** Subsystem functions never print directly to standard output.
They return status codes and invoke the `moss_log()` function for logging. This keeps the
core logic testable and ensures that the integration layer has full control over user-facing
output.

**Clean naming conventions.** All public functions are prefixed by their subsystem:
`sched_` for scheduling, `mem_` for memory management, and `sync_` for synchronization.
This prevents name collisions and makes the codebase self-documenting.

### 2.4 Build System

MOSS uses GNU Make with the following configuration:

- **Compiler:** `g++` with `-std=c++17`
- **Warning flags:** `-Wall -Wextra -Wno-unused-parameter`
- **Debug symbols:** `-g` for development and debugging
- **Include path:** `-Iinclude` to locate project headers

The Makefile provides three targets: `all` (default, builds the `moss` executable), `clean`
(removes all build artifacts), and `test` (compiles and runs the automated test suite).

---

## 3. Subsystem A: Process Management & CPU Scheduling

### 3.1 Overview

Subsystem A models the core process lifecycle and implements two CPU scheduling algorithms.
It is responsible for creating and terminating simulated processes, maintaining process
control blocks, managing the ready queue, and computing scheduling statistics including
waiting time and turnaround time.

### 3.2 Process Control Block

Every process in MOSS is represented by a Process Control Block (PCB), a structure
containing all metadata the system needs to manage the process:

| Field | Type | Purpose |
|-------|------|---------|
| `pid` | `int` | Unique process identifier, assigned sequentially |
| `name` | `char[32]` | Human-readable process name |
| `state` | `ProcessState` | Current state: NEW, READY, RUNNING, WAITING, TERMINATED |
| `priority` | `int` | Priority value (lower number = higher priority) |
| `arrival_time` | `int` | Time unit at which the process enters the system |
| `burst_time` | `int` | Total CPU time required by the process |
| `remaining_time` | `int` | CPU time still needed (decremented during execution) |
| `waiting_time` | `int` | Total time spent in the ready queue |
| `turnaround_time` | `int` | Total time from arrival to completion |
| `completion_time` | `int` | Time unit at which the process finished |
| `role` | `UserRole` | Access control role: ADMIN or USER |
| `active` | `int` | Whether this PCB slot is currently in use |

The process table is implemented as a fixed-size array of 32 PCB slots. This static
allocation avoids dynamic memory management bugs while providing sufficient capacity
for a simulator.

### 3.3 First-Come, First-Served (FCFS) Scheduling

FCFS is the simplest scheduling algorithm: processes are executed in the order they arrive,
and each process runs to completion before the next one begins. This is a
**non-preemptive** algorithm.

**Implementation details:**

1. All active, non-terminated processes are collected into a vector.
2. The vector is sorted by arrival time using `std::sort` with a lambda comparator.
   Ties are broken by PID (lower PID first).
3. The scheduler iterates through the sorted list. If the current time is before a
   process's arrival, an idle gap is recorded in the Gantt chart and time advances.
4. Each process runs for its full burst time. Its completion, waiting, and turnaround
   times are calculated upon completion.

**Characteristics:** FCFS is simple to implement and understand but suffers from the
**convoy effect** — short processes stuck behind a long-running process experience
disproportionately long wait times.

### 3.4 Round Robin (RR) Scheduling

Round Robin is a **preemptive** algorithm that gives each process a fixed time quantum.
If a process does not finish within its quantum, it is preempted and placed at the back
of the ready queue.

**Implementation details:**

1. Processes are sorted by arrival time and enqueued as they arrive.
2. A queue (implemented with `std::vector` and a front pointer) holds indices of
   ready processes.
3. At each step, the process at the front of the queue runs for `min(remaining_time, quantum)` time units.
4. After each quantum, any newly arrived processes are enqueued **before** the
   preempted process is re-enqueued — this ensures new arrivals are seen promptly.
5. If a process completes (remaining time reaches zero), it is removed from the queue
   and its statistics are calculated. Otherwise, it is placed at the back.

**Characteristics:** Round Robin is fairer than FCFS, especially for interactive
workloads. The choice of quantum is critical: too large and it degenerates to FCFS;
too small and context-switch overhead dominates.

### 3.5 Gantt Chart Visualization

After each scheduling run, MOSS produces a text-based Gantt chart showing the execution
timeline. Each cell is proportionally sized to the time duration it represents, and idle
periods (when no process is running) are explicitly marked. The timeline below the chart
shows time unit markers.

### 3.6 Statistics

After scheduling completes, MOSS calculates and displays per-process waiting time (time
spent in the ready queue), per-process turnaround time (time from arrival to completion),
and system-wide averages of both metrics. These statistics allow direct comparison between
FCFS and Round Robin on identical workloads.

---

## 4. Subsystem B: Memory Management & Virtual Memory

### 4.1 Overview

Subsystem B models a paged virtual memory system with demand paging, logical-to-physical
address translation, and configurable page replacement algorithms. It demonstrates how
an operating system maps a process's logical address space onto a limited number of
physical memory frames, and how it handles the inevitable conflicts when frames run out.

### 4.2 Configuration

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Page size | 256 bytes | Small enough to demonstrate replacement frequently |
| Logical address width | 16 bits | 64 KB address space, up to 256 logical pages |
| Physical frames | 8 | Small enough that eviction is triggered routinely |
| Max pages per process | 64 | Sufficient for demonstration purposes |

These values are defined as constants in `moss_common.h` and can be adjusted for
experimentation.

### 4.3 Address Translation

When a process accesses a logical address, MOSS translates it to a physical address
using the following decomposition:

```
Logical Address (16-bit):  [ Page Number (8 bits) | Offset (8 bits) ]

page_number   = logical_addr / PAGE_SIZE      (integer division)
offset        = logical_addr % PAGE_SIZE      (remainder)
physical_addr = frame_number * PAGE_SIZE + offset
```

For example, logical address `0x0120` with `PAGE_SIZE = 256`:
- Page number: `0x0120 / 256 = 1`
- Offset: `0x0120 % 256 = 32`
- If page 1 is in frame 3: physical address = `3 * 256 + 32 = 800 = 0x0320`

### 4.4 Demand Paging

MOSS implements **demand paging**: when memory is allocated to a process via
`mem_allocate()`, no physical frames are assigned. The page table entries are created
but marked as invalid. Only when a process actually accesses an address via
`mem_access()` does the system check whether the corresponding page is loaded.

If the page is already in a frame (valid bit is set), the access is a **page hit** and
translation proceeds immediately. If the page is not loaded, a **page fault** occurs:
the system must find a free frame (or evict an existing page) and load the requested page
into that frame.

This approach mirrors real operating systems, where demand paging reduces memory usage
by only loading pages that are actually needed.

### 4.5 Page Replacement Algorithms

When all physical frames are occupied and a page fault occurs, MOSS must evict a page
to make room. Two replacement algorithms are implemented:

**FIFO (First-In, First-Out).** The frame that was loaded earliest is evicted. MOSS
tracks loading order using a monotonically increasing counter (`load_order`) assigned to
each frame when it is loaded. The frame with the smallest `load_order` is the victim.

FIFO is simple and predictable, but it suffers from **Belady's anomaly** — increasing
the number of frames can paradoxically increase the page fault rate in certain access
patterns.

**LRU (Least Recently Used).** The frame that was accessed least recently is evicted.
MOSS tracks recency using a monotonically increasing counter (`last_access`) updated on
every access (both faults and hits). The frame with the smallest `last_access` is the
victim.

LRU generally outperforms FIFO because it exploits **temporal locality** — pages
accessed recently are likely to be accessed again soon. However, it requires tracking
access times for every memory reference, which adds overhead.

### 4.6 Statistics

MOSS tracks total memory accesses, total page faults, page hits, and the resulting hit
rate as a percentage. These statistics allow users to compare the effectiveness of FIFO
versus LRU on identical access patterns by resetting the system, switching algorithms
with `set_replacement`, and re-running the same sequence of accesses.

---

## 5. Subsystem C: Synchronization & Protection

### 5.1 Overview

Subsystem C addresses two fundamental operating system concerns: **synchronization**
(coordinating access to shared resources among concurrent processes) and **protection**
(enforcing security boundaries to prevent unauthorized operations). This subsystem
implements mutex locks, counting semaphores, two classical synchronization problems
(Producer-Consumer and Readers-Writers), role-based access control, and deadlock detection.

### 5.2 Mutex Locks

A mutex (mutual exclusion) lock ensures that only one process at a time can access a
critical section. MOSS implements mutexes with the following semantics:

- **Create:** Allocate a named mutex in an unlocked state.
- **Lock:** If the mutex is free, the requesting process acquires it immediately. If it
  is already held by another process, the requester is added to a FIFO wait queue and
  the call returns `MOSS_ERR_BUSY`.
- **Unlock:** Only the owning process can unlock the mutex. If the wait queue is non-empty,
  ownership is transferred directly to the next waiter (lock handoff) rather than
  releasing the lock to be re-acquired in a race.
- **Re-entrant protection:** If a process attempts to lock a mutex it already holds, the
  call returns `MOSS_ERR_BUSY` with a warning log, preventing self-deadlock.

### 5.3 Semaphores

Semaphores are a more general synchronization primitive than mutexes. A semaphore
maintains a non-negative integer value and supports two atomic operations:

- **Wait (P):** If the value is greater than zero, decrement it and proceed. If the
  value is zero, block the calling process by adding it to the semaphore's wait queue.
- **Signal (V):** If processes are waiting, wake the first waiter. Otherwise, increment
  the value.

MOSS implements counting semaphores, meaning the initial value can be greater than one.
This allows semaphores to control access to resources with multiple instances (such as a
bounded buffer with multiple slots).

### 5.4 Producer-Consumer Simulation

The Producer-Consumer problem is a classical synchronization challenge: a producer
process generates items and places them in a shared bounded buffer, while a consumer
process removes and processes items from the buffer. The challenge is to prevent the
producer from writing to a full buffer and the consumer from reading from an empty
buffer, all without race conditions.

MOSS solves this with three synchronization primitives:

- `sem_empty` (initial value = buffer_size): tracks available empty slots
- `sem_full` (initial value = 0): tracks available items ready to consume
- `mutex_buffer`: protects the buffer during insertion and removal

The simulation runs step-by-step, printing a detailed trace that shows each
wait/signal/lock/unlock operation, the current semaphore values, and the buffer
occupancy at every step. This trace makes the synchronization protocol visible and
comprehensible.

### 5.5 Readers-Writers Simulation

The Readers-Writers problem captures a fundamental access pattern in shared-memory systems:
multiple processes may read a shared resource simultaneously without conflict, but any
write requires exclusive access. The challenge is coordinating these two classes of
access efficiently and correctly.

MOSS implements a **reader-preference** policy using three simulated primitives:

- `rw_mutex`: controls exclusive access to the shared resource (binary, held by writers
  or by the reader group collectively).
- `mutex`: protects the `reader_count` variable during increment and decrement.
- `reader_count`: integer tracking how many readers are currently active.

**Protocol for readers:**
The first reader to arrive acquires `rw_mutex` on behalf of all readers, locking out
writers. Subsequent readers increment `reader_count` without re-acquiring `rw_mutex`.
When the last reader finishes, it releases `rw_mutex`, making the resource available to
a waiting writer.

**Protocol for writers:**
A writer must acquire `rw_mutex` exclusively. If readers are active or another writer
holds the lock, the writer is blocked and its state is reported in the simulation trace.

The simulation runs in alternating reader/writer steps, printing a step-by-step trace
that shows each lock acquisition, the current reader count, and whether a process
succeeds or is blocked. A summary at the end reports total reads and writes completed
with no data corruption.

**Reader preference trade-off:** Under reader preference, a continuous stream of readers
can starve writers indefinitely. The report discusses writer-preference as a potential
extension in Section 9.2. This design choice was intentional for demonstration purposes:
reader preference is simpler to trace and the starvation risk is visible in the output
when reader-heavy workloads are run.

### 5.6 Access Control Model

MOSS implements a role-based access control (RBAC) model with two roles and four action
types:

**Roles:** ADMIN (full privileges) and USER (restricted privileges).

**Resources:** `memory`, `process`, `sync_resource`.

**Actions:** `read`, `write`, `execute`, `admin`.

The access control matrix is defined as follows:

| Resource | Action | Admin | User |
|----------|--------|:-----:|:----:|
| memory | read | ✓ | ✓ |
| memory | write | ✓ | ✓ |
| memory | execute | ✓ | ✗ |
| memory | admin | ✓ | ✗ |
| process | read | ✓ | ✓ |
| process | write | ✓ | ✗ |
| process | execute | ✓ | ✓ |
| process | admin | ✓ | ✗ |
| sync_resource | read | ✓ | ✓ |
| sync_resource | write | ✓ | ✗ |
| sync_resource | execute | ✓ | ✓ |
| sync_resource | admin | ✓ | ✗ |

Permission checks query the process's role from its PCB (via `sched_get_process()`) and
look up the corresponding rule in the matrix. Unknown resource/action combinations are
denied by default, following the principle of least privilege. Processes can be promoted
or demoted at runtime using `sync_set_role()`.

### 5.7 Deadlock Detection

Deadlock occurs when a set of processes are each waiting for a resource held by another
process in the set, forming a circular dependency from which no process can proceed.

MOSS detects deadlocks by constructing a **wait-for graph** from the current mutex state.
For each mutex, if process A holds the lock and process B is in the wait queue, an edge
B → A is added to the graph (B is waiting for A). The system then applies **Floyd's
cycle detection algorithm** (tortoise and hare) to detect cycles in the graph.

If a cycle is found, MOSS reports the deadlock, identifying the specific processes
involved and the circular chain of dependencies. If no cycle exists, the system confirms
that no deadlock is present.

---

## 6. System Integration

### 6.1 Unified Command-Line Interface

The integration layer provides over 30 commands organized into four groups: process
management, memory management, synchronization and protection, and system commands.
The CLI reads input from standard input in a loop, parses each line into a command and
arguments, and dispatches to the appropriate handler function.

All commands follow a consistent pattern: validate inputs, call the subsystem API, check
the return code, and print a human-readable result. Error messages include the specific
error code to aid debugging.

### 6.2 System-Wide Logging

Every significant operation in MOSS is logged through the `moss_log()` function, which
prints a timestamped message with a severity level:

```
[14:23:07] [INFO]  Process created: PID=0 Name=WebServer Burst=5 Arrival=0 Role=ADMIN
[14:23:07] [WARN]  PAGE FAULT: PID=0, Logical=0x0010, Page=0
[14:23:07] [ERROR] Invalid memory access: PID=0, Addr=0x0F00 (page 15 out of range)
```

Logging provides visibility into the internal behavior of each subsystem without
violating the encapsulation principle, since subsystems call `moss_log()` rather than
printing directly.

### 6.3 Subsystem Interactions

The subsystems are not isolated silos; they interact through well-defined API calls:

- **Scheduler → Memory:** When a process is terminated via the CLI, the integration
  layer also calls `mem_free()` to release its physical frames.
- **Sync → Scheduler:** The `sync_check_permission()` function calls
  `sched_get_process()` to look up the process's role from its PCB. Similarly,
  `sync_set_role()` modifies the PCB through the same interface.
- **Integration → All:** The vertical slice demo in `main.cpp` orchestrates a complete
  flow through all three subsystems, demonstrating their interoperability.

### 6.4 Vertical Slice

The `demo_vertical` command runs a comprehensive end-to-end demonstration:

1. Three processes are created with different roles (Subsystem A).
2. Round Robin scheduling is executed and the Gantt chart is displayed (Subsystem A).
3. Memory is allocated and several addresses are accessed, triggering page faults and
   hits (Subsystem B).
4. A mutex is created, locked by one process, contested by another, and the lock
   handoff is demonstrated (Subsystem C).
5. Permission checks are performed showing both allowed and denied access (Subsystem C).
6. An invalid memory access is attempted and handled gracefully (error scenario).
7. A deliberate circular-wait deadlock is constructed and detected (Subsystem C).

This vertical slice satisfies the project requirement that "at least one system-level
error scenario must be handled and demonstrated."

---

## 7. Testing & Verification

### 7.1 Automated Test Suite

MOSS includes a comprehensive automated test suite in `tests/basic_tests.cpp` containing
78 individual test assertions organized into 16 test functions. The test framework is a
simple assertion-based system using a `TEST_ASSERT` macro that tracks pass/fail counts
and reports results.

The test suite provides its own implementation of `moss_log()` that suppresses output
during testing, allowing the tests to run cleanly without interleaved log messages.

### 7.2 Test Coverage

| Test Group | Tests | What is Verified |
|-----------|:-----:|-----------------|
| Process creation | 9 | Valid PID assignment, unique PIDs, PCB field correctness, invalid parameter rejection |
| Process termination | 4 | Successful termination, PCB removal, non-existent PID handling |
| FCFS scheduling | 1 | Successful completion with multiple processes |
| Round Robin scheduling | 3 | Successful completion, invalid quantum rejection |
| Empty scheduler | 2 | Graceful handling of scheduling with no processes |
| Memory allocation | 4 | Successful allocation, double-allocation prevention, invalid parameter rejection |
| Memory access | 5 | Page fault on first access, page hit on repeat, address translation, out-of-range rejection |
| FIFO page replacement | 10 | Frame filling, eviction of oldest page, re-loading after eviction |
| LRU page replacement | 1 | Correct eviction of least recently used page |
| Memory free | 3 | Successful free, double-free prevention, access-after-free prevention |
| Mutex operations | 8 | Lock/unlock, re-entrant detection, non-owner denial, lock handoff, invalid ID |
| Semaphore operations | 6 | Wait/signal, blocking at zero, wake-on-signal, invalid ID |
| Access control | 8 | Admin permissions, user restrictions, role changes, non-existent PID |
| Deadlock detection | 2 | No false positives, circular-wait detection |
| Producer-Consumer | 3 | Successful simulation, invalid parameter rejection |
| Vertical slice integration | 9 | End-to-end flow through all subsystems |

### 7.3 Algorithm Comparisons

To illustrate the behavioral differences between algorithms, we ran two controlled
experiments using the MOSS CLI.

#### Scheduling: FCFS vs. Round Robin

**Workload:** Four processes with varying burst and arrival times.

| Process | Burst | Arrival |
|---------|------:|-------:|
| P1 | 8 | 0 |
| P2 | 4 | 1 |
| P3 | 2 | 2 |
| P4 | 6 | 3 |

**Results:**

| Process | FCFS Waiting | FCFS Turnaround | RR (q=2) Waiting | RR (q=2) Turnaround |
|---------|------------:|----------------:|-----------------:|--------------------:|
| P1 | 0 | 8 | 10 | 18 |
| P2 | 7 | 11 | 7 | 11 |
| P3 | 10 | 12 | 2 | 4 |
| P4 | 11 | 17 | 11 | 17 |
| **Average** | **7.00** | **12.00** | **7.50** | **12.50** |

The results make the fairness trade-off concrete. P3 (the shortest job) has a turnaround
time of 12 under FCFS because it sits behind P1's 8-unit burst — the convoy effect. Under
Round Robin with quantum 2, P3 completes in 4 time units. P1 (the longest job) is the
inverse: FCFS gives it a turnaround of 8 (it runs first to completion), while Round Robin
extends it to 18 as it repeatedly yields the CPU. The averages are nearly identical (12.00
vs. 12.50), illustrating that Round Robin redistributes response time rather than reducing
total system load.

#### Memory: FIFO vs. LRU

**Workload:** A single process with 10 logical pages, 8 physical frames. The access
sequence models a working set of pages 0–2 (accessed frequently) alongside less-used
pages 3–9:

```
Pages accessed (in order):
0,1,2,3,4,5,6,7,  0,1,2,  8,9,  0,1,2,  3,4,5,6,7
└── fill frames ─┘ └─hit─┘      └─test─┘ └─fill─┘
```

**Results:**

| Metric | FIFO | LRU |
|--------|-----:|----:|
| Total accesses | 21 | 21 |
| Page faults | 18 | 15 |
| Page hits | 3 | 6 |
| Hit rate | 14.3% | 28.6% |

The difference emerges at access 12 (page 8). Both algorithms must evict a page from the
full 8-frame pool. FIFO evicts page 0 — the first page ever loaded — even though pages
0, 1, and 2 were just accessed at steps 9–11. When the access sequence returns to
pages 0, 1, and 2 three steps later, FIFO incurs three additional faults. LRU, by
contrast, evicts pages 3 and 4 (the least recently used at that point), keeping the
active working set resident. The result is six hits under LRU versus three under FIFO —
a 2× improvement in hit rate on this workload.

### 7.4 Build Verification

The project compiles with zero warnings under strict compiler flags:

```
g++ -Wall -Wextra -Wno-unused-parameter -std=c++17 -g -Iinclude
```

Both the main executable and the test runner compile and link successfully on macOS
(development) and Ubuntu Linux 22.04 LTS (target platform).

---

## 8. Design Decisions & Trade-offs

### 8.1 Static vs. Dynamic Allocation

**Decision:** All major data structures (process table, frame table, mutex array,
semaphore array) are statically allocated with fixed maximum sizes.

**Trade-off:** This eliminates the risk of memory leaks and dangling pointers, which are
common failure modes in systems-level C++ code. The cost is that the system has hard
limits (32 processes, 8 frames, 16 mutexes, 16 semaphores). For a simulator designed to
demonstrate concepts, these limits are generous; for a production system, dynamic
allocation with proper RAII would be necessary.

### 8.2 Simulated vs. Real Concurrency

**Decision:** All concurrency in MOSS is logically simulated. The Producer-Consumer
simulation alternates between producer and consumer actions in a single thread.
Scheduling runs to completion and records a timeline rather than preempting in real time.

**Trade-off:** This avoids introducing real race conditions and thread-safety concerns
into the simulator itself, which would obscure the concepts being demonstrated. The
trade-off is that MOSS cannot demonstrate timing-dependent phenomena like priority
inversion or actual thread starvation.

### 8.3 Demand Paging

**Decision:** Pages are not loaded into physical frames at allocation time; they are
loaded on first access.

**Trade-off:** This is more realistic — real operating systems use demand paging to
conserve physical memory. The downside is that the first access to every page incurs a
page fault, which in a real system would involve disk I/O latency. In MOSS, the fault
is instantaneous but still tracked for statistics.

### 8.4 Deadlock Detection vs. Prevention

**Decision:** MOSS implements deadlock **detection** (finding cycles after they form)
rather than deadlock **prevention** (structuring the system so cycles cannot form).

**Trade-off:** Detection is more flexible and allows the demonstration of actual deadlock
scenarios, which is valuable for an educational simulator. Prevention would require
imposing ordering constraints on lock acquisition, which would limit the scenarios that
can be demonstrated.

### 8.5 Header Naming Convention

**Decision:** Project headers are named `moss_common.h`, `moss_sched.h`, etc., rather
than the more natural `common.h`, `sched.h`.

**Rationale:** The POSIX standard defines a system header `<sched.h>` for thread
scheduling. When compiling C++ code with standard library headers like `<string>` or
`<algorithm>`, the compiler's include search path can find the project's `sched.h`
instead of the system header, causing compilation failures. Prefixing with `moss_`
eliminates this collision.

---

## 9. Known Limitations & Future Work

### 9.1 Current Limitations

- **No preemptive priority scheduling.** MOSS implements FCFS and Round Robin but not
  priority-based scheduling or multilevel feedback queues, which are important in real
  operating systems.
- **No disk I/O simulation.** Page faults are resolved instantaneously. A more realistic
  simulator would model disk access latency.
- **Static access control matrix.** The permission rules are compiled into the source code.
  A production system would load these from a configuration file.
- **No deadlock recovery.** MOSS detects deadlocks but does not resolve them (e.g., by
  terminating a process or rolling back a lock acquisition).
- **Single-threaded simulation.** All scheduling and synchronization is simulated
  logically rather than using actual threads.

### 9.2 Potential Extensions

For a graduate-level (COSC 514) implementation, the following extensions could be added:

- **Priority scheduling with preemption and aging** to prevent starvation.
- **Multilevel Feedback Queue** scheduler that dynamically adjusts process priority.
- **Optimal page replacement** (Belady's algorithm) for theoretical comparison against FIFO and LRU.
- **Working Set page replacement** that adapts to each process's memory access pattern.
- **Writer-preference Readers-Writers** variant to demonstrate writer starvation under reader preference.
- **Configurable access control** loaded from external policy files at runtime.

---

## 10. Conclusion

MOSS successfully demonstrates the core concepts of operating system design in a
practical, interactive simulator. The three subsystems — scheduling, memory management,
and synchronization — are implemented as modular, well-documented components that
communicate through clean APIs and interact meaningfully in the integrated system.

The project meets all specified requirements: two scheduling algorithms (FCFS and Round
Robin) with Gantt chart visualization, paged virtual memory with two replacement algorithms
(FIFO and LRU), mutex locks and semaphores with both Producer-Consumer and Readers-Writers
simulations, role-based access control with permission enforcement, and deadlock detection
via wait-for graph cycle analysis. Controlled experiments quantified the behavioral
differences: Round Robin reduced P3's turnaround from 12 to 4 time units (3×) relative to
FCFS on the same workload; LRU achieved a 28.6% hit rate versus 14.3% for FIFO on a
working-set access pattern, cutting page faults from 18 to 15. The unified command-line
interface provides interactive access to all functionality, and the vertical slice
demonstration shows a complete end-to-end flow through all subsystems.

The automated test suite of 78 tests verifies correctness across all subsystems and their
integration, providing confidence that the system behaves as designed. The project compiles
with zero warnings and runs on the required Ubuntu Linux 22.04 LTS platform.

Building MOSS reinforced several key lessons: that modular design with clean interfaces
pays dividends during integration, that consistent error handling prevents silent failures,
and that even "simple" algorithms like Round Robin have subtle implementation details
(such as the ordering of new arrivals versus preempted processes in the ready queue) that
require careful thought.

Operating systems remain among the most fascinating areas of computer science precisely
because every design choice involves trade-offs — fairness versus throughput, simplicity
versus performance, flexibility versus safety. MOSS provides a window into these
trade-offs and a foundation for deeper exploration.

---

## References

1. Silberschatz, A., Galvin, P. B., & Gagne, G. *Operating System Concepts*, 10th Edition.
   Wiley, 2018.
2. Tanenbaum, A. S., & Bos, H. *Modern Operating Systems*, 4th Edition. Pearson, 2014.
3. GCC/G++ Documentation. GNU Compiler Collection. https://gcc.gnu.org/
4. POSIX.1-2017 Standard. The Open Group. https://pubs.opengroup.org/onlinepubs/9699919799/

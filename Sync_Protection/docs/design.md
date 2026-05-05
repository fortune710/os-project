# Subsystem C: Synchronization & Protection — Design Document

## Overview

This subsystem implements synchronization primitives and protection mechanisms for the
MOSS simulator. It provides mutex locks with wait queues, counting semaphores, a
Producer-Consumer simulation, role-based access control, and deadlock detection via
wait-for graph cycle detection.

## Design Decisions

### Mutex Implementation

Mutexes are implemented as named objects with an owner PID and a FIFO wait queue. When a
process locks a mutex that is already held, it is added to the queue and the call returns
`MOSS_ERR_BUSY`. When the owner unlocks, the lock is handed directly to the next waiter
rather than being released to the pool — this prevents starvation and ensures fairness.

Re-entrant locking (a process trying to lock a mutex it already holds) is detected and
returns an error rather than deadlocking the process.

### Semaphore Implementation

Semaphores maintain an integer value and a wait queue. The Wait (P) operation decrements
the value if positive, otherwise blocks the caller. The Signal (V) operation wakes a
blocked process if any are waiting, otherwise increments the value. This correctly
implements counting semaphore semantics.

### Producer-Consumer

The bounded-buffer problem is solved using two counting semaphores (`empty` and `full`)
and a mutex (`buffer_lock`). The simulation runs in a single thread, alternating between
producer and consumer steps, and printing a detailed trace of every synchronization
operation. This makes the protocol visible without requiring actual threading.

### Access Control

I implemented a static access control matrix with two roles (ADMIN, USER) and four
action types (read, write, execute, admin) across three resources (memory, process,
sync_resource). The matrix is defined as a constant array of rules, and permission
checks iterate through the rules to find a match. Unknown combinations are denied by
default, following the principle of least privilege.

The access control system depends on the scheduler subsystem to look up process roles
from the PCB via `sched_get_process()`. This is the primary cross-subsystem dependency.

### Deadlock Detection

Deadlock detection constructs a wait-for graph from the current mutex state: for each
mutex, if process A holds it and process B is waiting, an edge B → A is added. Floyd's
tortoise-and-hare algorithm then checks for cycles. If a cycle is found, the deadlocked
processes are reported.

I chose detection over prevention because it allows demonstrating actual deadlock
scenarios, which is more educational than preventing them.

## Assumptions

- Maximum 16 mutexes and 16 semaphores.
- Wait queues hold up to 32 processes.
- Access control matrix is compiled into the binary (not configurable at runtime).
- All concurrency is logically simulated — no real threads.

## Limitations

- No deadlock recovery (only detection).
- No condition variables.
- No readers-writers lock.
- Access control matrix cannot be modified at runtime (only roles can change).
- Producer-Consumer simulation is deterministic (producer always goes first).

## API Summary

| Function | Purpose |
|----------|---------|
| `sync_init()` | Initialize the sync subsystem |
| `sync_mutex_create()` | Create a named mutex |
| `sync_mutex_lock()` | Lock a mutex |
| `sync_mutex_unlock()` | Unlock a mutex |
| `sync_sem_create()` | Create a semaphore |
| `sync_sem_wait()` | Semaphore wait (P) |
| `sync_sem_signal()` | Semaphore signal (V) |
| `sync_run_producer_consumer()` | Run bounded-buffer simulation |
| `sync_check_permission()` | Check access control |
| `sync_set_role()` | Change a process's role |
| `sync_detect_deadlock()` | Detect deadlock via wait-for graph |
| `sync_cleanup()` | Free all resources |

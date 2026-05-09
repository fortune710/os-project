# MOSS API Reference

This document provides a complete API reference for all three MOSS subsystems.

---

## Error Codes

All API functions return `0` (`MOSS_SUCCESS`) on success and negative values on error:

| Code | Constant | Meaning |
|------|----------|---------|
| `0` | `MOSS_SUCCESS` | Operation completed successfully |
| `-1` | `MOSS_ERR_NOT_FOUND` | Resource or process not found |
| `-2` | `MOSS_ERR_FULL` | Resource limit reached (table full) |
| `-3` | `MOSS_ERR_INVALID` | Invalid parameter or operation |
| `-4` | `MOSS_ERR_DENIED` | Permission denied / access violation |
| `-5` | `MOSS_ERR_DEADLOCK` | Deadlock detected |
| `-6` | `MOSS_ERR_BUSY` | Resource is busy / locked |
| `-7` | `MOSS_ERR_NO_MEM` | Out of memory |

---

## Subsystem A: Process Management & CPU Scheduling (`sched_`)

### `int sched_init(void)`
Initialize the scheduler subsystem. Must be called before any other `sched_` functions.

**Returns**: `MOSS_SUCCESS`

---

### `int sched_create_process(const char *name, int burst, int arrival, int priority, UserRole role)`
Create a new process and add it to the process table.

**Parameters**:
| Name | Type | Description |
|------|------|-------------|
| `name` | `const char*` | Process name (max 32 chars) |
| `burst` | `int` | CPU burst time (must be > 0) |
| `arrival` | `int` | Arrival time (must be >= 0) |
| `priority` | `int` | Priority value (lower = higher priority) |
| `role` | `UserRole` | `ROLE_ADMIN` or `ROLE_USER` |

**Returns**: PID (>= 0) on success, or:
- `MOSS_ERR_INVALID` — NULL name, burst <= 0, or arrival < 0
- `MOSS_ERR_FULL` — process table full (max 32 processes)

---

### `int sched_terminate_process(int pid)`
Terminate a process and remove it from the process table.

**Parameters**: `pid` — Process ID  
**Returns**: `MOSS_SUCCESS` or `MOSS_ERR_NOT_FOUND`

---

### `int sched_run_fcfs(void)`
Execute First-Come, First-Served scheduling on all ready processes.
Processes are sorted by arrival time and run non-preemptively.

**Side Effects**: Updates Gantt chart, calculates waiting/turnaround times, marks processes as terminated.  
**Returns**: `MOSS_SUCCESS` or `MOSS_ERR_INVALID` (no ready processes)

---

### `int sched_run_rr(int quantum)`
Execute Round Robin scheduling with the specified time quantum.

**Parameters**: `quantum` — time quantum (must be > 0)  
**Side Effects**: Same as `sched_run_fcfs()`  
**Returns**: `MOSS_SUCCESS` or `MOSS_ERR_INVALID`

---

### `int sched_get_running(void)`
Get the PID of the currently running process.

**Returns**: PID (>= 0) or `MOSS_ERR_NOT_FOUND` if no process is running.

---

### `PCB* sched_get_process(int pid)`
Get a pointer to a process's PCB.

**Parameters**: `pid` — Process ID  
**Returns**: Pointer to PCB, or `NULL` if not found.

---

### `void sched_print_gantt(void)`
Print the Gantt chart of the last scheduling run to stdout.

---

### `void sched_print_stats(void)`
Print per-process and average waiting/turnaround time statistics.

---

### `int sched_list_processes(void)`
Print a table of all active processes and their states.

**Returns**: `MOSS_SUCCESS`

---

### `void sched_cleanup(void)`
Free all scheduler resources and reset state.

---

## Subsystem B: Memory Management & Virtual Memory (`mem_`)

### `int mem_init(void)`
Initialize the memory management subsystem (8 frames, 256-byte pages, 16-bit address space).

**Returns**: `MOSS_SUCCESS`

---

### `int mem_allocate(int pid, int num_pages)`
Allocate logical address space for a process.

**Parameters**:
| Name | Type | Description |
|------|------|-------------|
| `pid` | `int` | Process ID (must exist) |
| `num_pages` | `int` | Number of pages (1–64) |

**Returns**: `MOSS_SUCCESS`, `MOSS_ERR_INVALID` (already allocated, bad params), or `MOSS_ERR_FULL`

**Note**: Pages are not loaded into physical frames until accessed (demand paging).

---

### `int mem_access(int pid, uint16_t logical_addr)`
Access a logical address, performing address translation.

**Parameters**:
| Name | Type | Description |
|------|------|-------------|
| `pid` | `int` | Process ID |
| `logical_addr` | `uint16_t` | 16-bit logical address |

**Returns**: Physical address (>= 0) on success, or:
- `MOSS_ERR_NOT_FOUND` — no memory allocated for PID
- `MOSS_ERR_INVALID` — address out of range

**Side Effects**: May trigger page fault and page replacement.

**Address Translation**:
- `page_number = logical_addr / PAGE_SIZE`
- `offset = logical_addr % PAGE_SIZE`
- `physical_addr = frame_number * PAGE_SIZE + offset`

---

### `int mem_free(int pid)`
Free all memory allocated to a process.

**Returns**: `MOSS_SUCCESS` or `MOSS_ERR_NOT_FOUND`

---

### `int mem_set_replacement(const char *algo)`
Set the page replacement algorithm.

**Parameters**: `algo` — `"FIFO"` or `"LRU"`  
**Returns**: `MOSS_SUCCESS` or `MOSS_ERR_INVALID`

---

### `void mem_print_page_table(int pid)`
Print the page table for a specific process.

### `void mem_print_frames(void)`
Print the current state of all physical frames.

### `void mem_print_stats(void)`
Print memory statistics (accesses, faults, hit rate).

### `void mem_cleanup(void)`
Free all memory subsystem resources and reset state.

---

## Subsystem C: Synchronization & Protection (`sync_`)

### `int sync_init(void)`
Initialize the synchronization subsystem.

**Returns**: `MOSS_SUCCESS`

---

### `int sync_mutex_create(const char *name)`
Create a named mutex.

**Parameters**: `name` — mutex name (max 32 chars)  
**Returns**: Resource ID (>= 0) or `MOSS_ERR_FULL`

---

### `int sync_mutex_lock(int resource_id, int pid)`
Lock a mutex for a process.

**Returns**:
- `MOSS_SUCCESS` — lock acquired
- `MOSS_ERR_BUSY` — already locked (process added to wait queue)
- `MOSS_ERR_NOT_FOUND` — invalid resource ID

---

### `int sync_mutex_unlock(int resource_id, int pid)`
Unlock a mutex. Only the owning process can unlock.

**Returns**:
- `MOSS_SUCCESS` — unlocked (or handed to next waiter)
- `MOSS_ERR_DENIED` — caller is not the lock owner
- `MOSS_ERR_NOT_FOUND` — invalid resource ID

---

### `int sync_sem_create(const char *name, int initial_value)`
Create a named semaphore with an initial value.

**Returns**: Semaphore ID (>= 0) or error code.

---

### `int sync_sem_wait(int sem_id, int pid)`
Perform P() (wait/decrement) on a semaphore.

**Returns**:
- `MOSS_SUCCESS` — value decremented
- `MOSS_ERR_BUSY` — value was 0, process blocked

---

### `int sync_sem_signal(int sem_id, int pid)`
Perform V() (signal/increment) on a semaphore.

**Returns**: `MOSS_SUCCESS` (wakes a waiter if any, otherwise increments)

---

### `int sync_run_producer_consumer(int buffer_size, int num_items)`
Simulate the Producer-Consumer problem with a bounded buffer.

**Parameters**:
| Name | Type | Description |
|------|------|-------------|
| `buffer_size` | `int` | Bounded buffer size (1–16) |
| `num_items` | `int` | Items to produce/consume (> 0) |

**Output**: Step-by-step trace printed to stdout.

---

### `int sync_check_permission(int pid, const char *resource, const char *action)`
Check if a process has permission to perform an action on a resource.

**Resources**: `"memory"`, `"process"`, `"sync_resource"`  
**Actions**: `"read"`, `"write"`, `"execute"`, `"admin"`

**Access Matrix**:
| Resource | Action | Admin | User |
|----------|--------|-------|------|
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

**Returns**: `MOSS_SUCCESS` (granted) or `MOSS_ERR_DENIED`

---

### `int sync_set_role(int pid, UserRole role)`
Set the access control role for a process.

**Returns**: `MOSS_SUCCESS` or `MOSS_ERR_NOT_FOUND`

---

### `int sync_detect_deadlock(void)`
Detect deadlock by analyzing the wait-for graph for cycles.

**Returns**: `MOSS_ERR_DEADLOCK` if cycle found, `MOSS_SUCCESS` otherwise.

---

### `void sync_print_state(void)`
Print all mutexes and semaphores with their current state.

### `void sync_cleanup(void)`
Free all synchronization resources and reset state.

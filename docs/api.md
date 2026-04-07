# Subsystem Interface Specification (MOSS)

This specification defines the public APIs, shared data structures, and constraints that
all subsystems must follow to enable integration.

## API Principles & Conventions
- APIs are **stable, minimal, and documented**.
- Subsystems expose **function calls**, not internal data structures.
- **Global variables shared across subsystems are discouraged**.
- **No printing inside core API functions**. Return status codes and let the main system
  handle output/logging.
- Return values:
  - `0` for success
  - Negative values for errors (e.g., `-1`, `-2`)
- Internal OS/library calls may use `errno` for diagnostics, but **public APIs still return
  negative codes**.

## Common Types & Error Codes
```c
typedef int moss_status_t;

typedef enum {
    MOSS_OK = 0,
    MOSS_EINVAL = -1,
    MOSS_ENOMEM = -2,
    MOSS_ENOTREADY = -3,
    MOSS_ESTATE = -4,
    MOSS_EPERM = -5
} moss_error_t;
```

Common data types used across subsystems:
- `moss_pid_t` (process IDs)
- `moss_time_t` (simulation time units)
- `mem_access_t` (memory access request)
- `sync_access_t` (protection request)

## Subsystem A – Process Management & CPU Scheduling (sched_)
**Responsibilities:**
- Process creation/termination
- Ready queue maintenance
- CPU scheduling (FCFS + RR; add Priority or MLFQ for graduate)
- Scheduling timeline and waiting/turnaround time

**Public API:**
```c
moss_status_t sched_init(sched_policy_t policy, moss_time_t quantum);
moss_status_t sched_create_process(const sched_process_t *proc);
moss_status_t sched_terminate_process(moss_pid_t pid);
moss_status_t sched_tick(moss_time_t delta);

moss_status_t sched_schedule(moss_pid_t *scheduled_pid);
moss_status_t sched_get_stats(sched_stats_t *out_stats);
moss_status_t sched_set_policy(sched_policy_t policy, moss_time_t quantum);
```

**Inputs / Outputs:**
- Inputs are validated; invalid parameters return `MOSS_EINVAL`.
- `sched_schedule` returns the PID chosen for execution.
- `sched_get_stats` returns cumulative stats; caller owns `out_stats`.

**Ownership & State:**
- Scheduler owns its ready queue and PCB data.
- Other subsystems must not mutate scheduler state directly.

## Subsystem B – Memory Management & Virtual Memory (mem_)
**Responsibilities:**
- Logical → physical translation (fixed page size)
- Page replacement (FIFO + LRU; add Optimal or Working Set for graduate)
- Page fault tracking and visualization

**Public API:**
```c
moss_status_t mem_init(const mem_config_t *cfg);
moss_status_t mem_configure(const mem_config_t *cfg);
moss_status_t mem_access(const mem_access_t *access,
                          uint64_t *out_physical_address,
                          int *out_page_fault);
size_t mem_page_faults(void);
moss_status_t mem_set_policy(mem_policy_t policy);
moss_status_t mem_reset(void);
```

**Inputs / Outputs:**
- `mem_access` sets `out_page_fault` to 1 on fault, 0 otherwise.
- Translation errors return `MOSS_ESTATE` or `MOSS_EINVAL`.

**Ownership & State:**
- Memory subsystem owns page tables and frame state.
- No external mutation of internal tables.

## Subsystem C – Synchronization & Protection (sync_)
**Responsibilities:**
- Mutexes and semaphores
- Simulate classical problems (Producer–Consumer or Readers–Writers)
- Basic access control model (roles + permissions)

**Public API:**
```c
moss_status_t sync_init(void);

moss_status_t sync_mutex_create(const char *name);
moss_status_t sync_mutex_lock(const char *name);
moss_status_t sync_mutex_unlock(const char *name);

moss_status_t sync_sem_create(const char *name, int initial_count);
moss_status_t sync_sem_wait(const char *name);
moss_status_t sync_sem_post(const char *name);

moss_status_t sync_check_access(const sync_access_t *access);
moss_status_t sync_run_scenario(const char *scenario_name);
```

**Inputs / Outputs:**
- Invalid handles or names return `MOSS_EINVAL`.
- Permission errors return `MOSS_EPERM`.

**Ownership & State:**
- Sync subsystem owns lock/sem tables and protection state.
- Access checks must go through `sync_check_access`.

## Shared Structures & Assumptions
- Shared data structures are defined in public headers under `include/`.
- All state changes occur **only via API calls**.
- The main system orchestrates calls and handles logging/output.

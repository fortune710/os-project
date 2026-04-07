# Public API Guide (Beginner-Friendly)

This document explains the public APIs and error codes used by MOSS in plain language.

## How to Read the API
- Every public function returns a **status code** (`moss_status_t`).
- **`0` means success**.
- Any **negative value means failure**.
- Functions never print output directly; the caller decides what to log or display.

## Standard Error Codes
These codes are shared by all subsystems:

```c
MOSS_OK        =  0   // success
MOSS_EINVAL    = -1   // invalid argument (bad pointer, out-of-range value)
MOSS_ENOMEM    = -2   // not enough memory or frames
MOSS_ENOTREADY = -3   // subsystem not initialized or not configured
MOSS_ESTATE    = -4   // invalid internal state (e.g., empty queue when scheduling)
MOSS_EPERM     = -5   // permission denied (protection checks failed)
```

## Where These Types Live
- Common types and error codes are defined in `include/moss.h`.
- Subsystem headers include `moss.h` so everything stays consistent.

## Scheduler (sched_)
**Purpose:** decide which process runs next and track timing statistics.

Key functions:
- `sched_init(policy, quantum)`
  - Sets scheduling policy (FCFS/RR/etc.).
  - Returns `MOSS_ENOTREADY` if needed state is missing.
- `sched_create_process(proc)`
  - Adds a process to the ready queue.
- `sched_schedule(&pid)`
  - Picks the next PID to run.
  - Returns `MOSS_ESTATE` if no process is ready.
- `sched_get_stats(&stats)`
  - Returns waiting/turnaround totals.

## Memory (mem_)
**Purpose:** translate logical addresses, simulate paging, track page faults.

Key functions:
- `mem_init(cfg)` / `mem_configure(cfg)`
  - Sets page size, number of frames, and replacement policy.
- `mem_access(access, &phys_addr, &page_fault)`
  - Converts a logical address to a physical address.
  - Sets `page_fault` to 1 if a fault occurred.
- `mem_page_faults()`
  - Returns total page faults so far.

## Synchronization & Protection (sync_)
**Purpose:** simulate locks/semaphores and basic access control.

Key functions:
- `sync_mutex_*` / `sync_sem_*`
  - Create and operate on locks and semaphores.
- `sync_check_access(access)`
  - Enforces role-based permissions.
  - Returns `MOSS_EPERM` if access is denied.
- `sync_run_scenario(name)`
  - Runs a predefined synchronization demo (e.g., producer–consumer).

## Example: Handling Errors
```c
moss_status_t rc = mem_access(&req, &phys, &fault);
if (rc != MOSS_OK) {
    // handle error in main system
}
```

## Where the APIs Live
- Headers: `include/sched.h`, `include/mem.h`, `include/sync.h`
- Spec: `docs/api.md`

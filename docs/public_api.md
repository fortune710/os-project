# Public API Guide (Beginner-Friendly)

This document explains the public APIs and error codes used by MOSS in plain language.

## Standard Error Codes
```c
MOSS_OK        =  0
MOSS_EINVAL    = -1
MOSS_ENOMEM    = -2
MOSS_ENOTREADY = -3
MOSS_ESTATE    = -4
MOSS_EPERM     = -5
```

## Scheduler (`scheduler_`)
Purpose: simulate process lifecycle and CPU selection.

### Lifecycle States
- `PROCESS_LIFECYCLE_NEW`
- `PROCESS_LIFECYCLE_READY`
- `PROCESS_LIFECYCLE_RUNNING`
- `PROCESS_LIFECYCLE_WAITING`
- `PROCESS_LIFECYCLE_TERMINATED`

The lifecycle enum and process control block definition are declared in `include/scheduler.h`.

### Algorithms
- `SCHEDULER_ALGORITHM_FCFS`
- `SCHEDULER_ALGORITHM_ROUND_ROBIN`
- `SCHEDULER_ALGORITHM_PRIORITY`
- `SCHEDULER_ALGORITHM_MLFQ`

### Public Functions
- `scheduler_init(scheduling_algorithm, algorithm_time_quantum)`
- `scheduler_create_process(scheduler_process_definition)`
- `scheduler_schedule(scheduled_process_identifier)`
- `scheduler_tick(time_delta)`
- `scheduler_terminate_process(process_identifier)`
- `scheduler_get_statistics(output_statistics)`
- `scheduler_set_algorithm(scheduling_algorithm, algorithm_time_quantum)`

### Ownership Rule
Scheduler owns ready queue state and all process control block lifecycle transitions.

## Memory (`mem_`)
Purpose: logical-to-physical translation and page-fault simulation.

## Synchronization and Protection (`sync_`)
Purpose: mutex/semaphore simulation and access checks.

## Header Locations
- `include/scheduler.h`
- `include/mem.h`
- `include/sync.h`

# Subsystem Interface Specification (MOSS)

This specification defines public APIs, shared structures, and subsystem responsibilities.

## Common Return Convention
- `0` means success.
- Negative values are errors (`MOSS_EINVAL`, `MOSS_ENOTREADY`, `MOSS_ESTATE`, and others).

## Subsystem A – Process Management and CPU Scheduling (`scheduler_`)
### Responsibilities
- Process creation and termination
- Ready queue maintenance
- Algorithm-based CPU selection
- Waiting and turnaround tracking
- Process control block lifecycle ownership

### Public API
```c
moss_status_t scheduler_init(scheduler_algorithm scheduling_algorithm, moss_time_t algorithm_time_quantum);
moss_status_t scheduler_create_process(const scheduler_process *scheduler_process_definition);
moss_status_t scheduler_terminate_process(moss_pid_t process_identifier);
moss_status_t scheduler_tick(moss_time_t time_delta);
moss_status_t scheduler_schedule(moss_pid_t *scheduled_process_identifier);
moss_status_t scheduler_get_statistics(scheduler_statistics *output_statistics);
moss_status_t scheduler_set_algorithm(scheduler_algorithm scheduling_algorithm, moss_time_t algorithm_time_quantum);
```

### Algorithm Execution API
```c
moss_status_t scheduler_run_fcfs(...);
moss_status_t scheduler_run_round_robin(..., moss_time_t algorithm_time_quantum, ...);
moss_status_t scheduler_run_mlfq(...);
```

Algorithm execution outputs:
- `scheduler_gantt_segment`
- `scheduler_process_metrics`
- `scheduler_algorithm_summary`

### Lifecycle Model
- `PROCESS_LIFECYCLE_NEW`
- `PROCESS_LIFECYCLE_READY`
- `PROCESS_LIFECYCLE_RUNNING`
- `PROCESS_LIFECYCLE_WAITING`
- `PROCESS_LIFECYCLE_TERMINATED`

Milestone transitions:
- Creation: `NEW -> READY`
- Dispatch: `READY -> RUNNING`
- Explicit termination: `RUNNING/READY/WAITING -> TERMINATED` then removed from active storage
- Completion on remaining time exhaustion: `RUNNING -> TERMINATED` then removed from active storage

### Algorithm Enum
- `SCHEDULER_ALGORITHM_FCFS`
- `SCHEDULER_ALGORITHM_ROUND_ROBIN`
- `SCHEDULER_ALGORITHM_PRIORITY`
- `SCHEDULER_ALGORITHM_MLFQ`

### Ownership Rules
- Scheduler owns process control block storage and lifecycle transitions.
- External modules must not mutate scheduler internals directly.
- Scheduling algorithm logic and metric computation are implemented in `src/scheduler/algorithms.cpp`.

## Subsystem B – Memory Management (`mem_`)
### Public API
```c
moss_status_t mem_init(const mem_config_t *cfg);
moss_status_t mem_configure(const mem_config_t *cfg);
moss_status_t mem_access(const mem_access_t *access, uint64_t *out_physical_address, int *out_page_fault);
size_t mem_page_faults(void);
moss_status_t mem_set_policy(mem_policy_t policy);
moss_status_t mem_reset(void);
```

## Subsystem C – Synchronization and Protection (`sync_`)
### Public API
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

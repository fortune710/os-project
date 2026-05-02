# MOSS Subsystem C Design (Synchronization and Protection)

## 1. Architectural Overview

This module implements Subsystem C for the MOSS simulator. It provides:

- Synchronization primitives: mutexes and counting semaphores.
- A classical synchronization scenario: Producer-Consumer.
- A basic protection model: role assignment and permission checks.

The subsystem exposes a clean API in `include/sync.h` and keeps implementation details in `src/sync/sync.c`.

## 2. Public API Reference

### Mutex API

- `int sync_mutex_init(int mutex_id);`
- `int sync_mutex_lock(int mutex_id);`
- `int sync_mutex_unlock(int mutex_id);`

### Semaphore API

- `int sync_sem_init(int sem_id, int initial_value);`
- `int sync_sem_wait(int sem_id);`
- `int sync_sem_post(int sem_id);`

### Protection API

- `int sync_set_process_role(int process_id, int role_id);`
- `int sync_check_permission(int process_id, int resource_id);`

### Producer-Consumer API

- `int sync_pc_init(void);`
- `int sync_pc_reset(void);`
- `int sync_produce(int item);`
- `int sync_consume(int *item);`

All APIs return `0` on success and negative values on failure.

## 3. Implementation Decisions

- Internal module state is file-local (`static`) in `sync.c`.
- Resource pools are bounded by fixed constants (`MAX_MUTEXES`, `MAX_SEMAPHORES`, `MAX_PROCESSES`).
- Producer-Consumer uses:
  - One mutex to protect shared buffer indices.
  - `empty` semaphore initialized to buffer size.
  - `full` semaphore initialized to zero.
- `sync_pc_reset()` is provided to support repeated benchmark runs with consistent initial state.
- `main.c` supports two modes:
  - `demo`: human-readable threaded demonstration.
  - `bench`: CSV-style benchmark output for quantitative analysis.

## 4. Quantitative Evaluation Plan and Results

Run benchmark:

```bash
make
./moss_sim bench
```

Output format:

```text
iterations,total_ms,avg_op_us,producer_failures,consumer_failures
```

Recommended workloads:

- 1,000 iterations
- 10,000 iterations
- 100,000 iterations

### Measured Results (macOS dev machine)

Fill this table with your observed output:

| Iterations | Total ms | Avg us/op | Producer failures | Consumer failures |
| --- | ---: | ---: | ---: | ---: |
| 1,000 | 1.625 | 0.812 | 0 | 0 |
| 10,000 | 17.198 | 0.860 | 0 | 0 |
| 100,000 | 141.906 | 0.710 | 0 | 0 |

### Analysis Notes (COSC 514)

When writing the final report, discuss:

- How total runtime scales as workload increases.
- Whether average per-operation cost remains stable.
- Sources of overhead: lock contention, context switching, semaphore operations.
- Differences between macOS and Ubuntu measurements (if any).

## 5. Build and Execution

Build:

```bash
make
```

Run demo:

```bash
./moss_sim demo
```

Run benchmark:

```bash
./moss_sim bench
```

Clean:

```bash
make clean
```

## 6. Current Limitations

- Protection model is intentionally minimal (role threshold check by resource parity).
- No teardown APIs for full resource lifecycle across all pool objects.
- This is a single subsystem and does not yet include process scheduling or virtual memory integration.

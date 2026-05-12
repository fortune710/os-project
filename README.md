# MOSS – Mini Operating System Services Simulator

MOSS is a C++17 command-line simulator that integrates three operating-system subsystems:

- **Process Management & CPU Scheduling** (FCFS and Round Robin)
- **Memory Management & Virtual Memory** (paging with FIFO/LRU replacement)
- **Synchronization & Protection** (mutexes, semaphores, access control, deadlock detection)

## Project Structure

- `src/main.cpp` – unified CLI and logger
- `src/sched/sched.cpp` – scheduler subsystem
- `src/mem/mem.cpp` – memory subsystem
- `src/sync/sync.cpp` – synchronization subsystem
- `include/` – public subsystem headers and shared types/constants
- `tests/test_sync.cpp` – integration-focused sync subsystem test target
- `docs/` – design, API, and report documentation

## Requirements

- `g++` with C++17 support
- `make`

## Build

```bash
make
```

This produces the executable:

- `sync_demo`

## Run

```bash
./sync_demo
```

## Run Tests

```bash
make test
```

This builds and runs:

- `test_sync`

## CLI Commands

Use `help` inside the simulator to list all commands.

Major command groups:

- **Scheduling**: `create_process`, `list_processes`, `terminate`, `schedule`, `gantt`, `stats`
- **Memory**: `alloc_memory`, `access_memory`, `free_memory`, `set_replacement`, `page_table`, `frames`, `mem_stats`
- **Synchronization**: `create_mutex`, `lock`, `unlock`, `create_sem`, `sem_wait`, `sem_signal`, `run_pc`, `run_rw`, `detect_deadlock`, `sync_state`
- **Access control**: `set_role`, `check_perm`
- **System**: `demo_vertical`, `help`, `exit`

## Notes

- Subsystems are initialized and cleaned up by the unified `main` entrypoint.
- Scheduler process limits and common constants are defined in `include/moss_common.h`.

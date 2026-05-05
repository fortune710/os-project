# MOSS Design Notes

## Scheduler Internals

The scheduler subsystem stores process control blocks in a map keyed by process identifier, tracks a ready queue, tracks one running process identifier, and accumulates global scheduler statistics.

Algorithm execution and metric calculations are isolated in `src/scheduler/algorithms.cpp`, while lifecycle/state APIs remain in `src/scheduler/scheduler.cpp`.

## Process Control Block Model

The following definitions now live in `include/scheduler.h`:
- `process_lifecycle`
- `process_control_block`

Lifecycle values:
- `PROCESS_LIFECYCLE_NEW`
- `PROCESS_LIFECYCLE_READY`
- `PROCESS_LIFECYCLE_RUNNING`
- `PROCESS_LIFECYCLE_WAITING`
- `PROCESS_LIFECYCLE_TERMINATED`

`PROCESS_LIFECYCLE_WAITING` remains part of the model now so future synchronization and blocking behavior can integrate without changing the core lifecycle contract.

## Lifecycle Transitions Implemented
- Process creation transitions `NEW -> READY`.
- Dispatch transitions `READY -> RUNNING`.
- Termination transitions `RUNNING/READY/WAITING -> TERMINATED`.
- Completed running processes transition `RUNNING -> TERMINATED`.

After a process reaches terminated state, scheduler statistics are aggregated and the process control block is removed from active storage.

## Naming Direction

Scheduler APIs use the full `scheduler_` prefix and use `algorithm` terminology instead of `policy`.

## Algorithm Reporting Model

Scheduling runs provide:
- Gantt chart segments (`scheduler_gantt_segment`)
- Per-process waiting/turnaround/completion metrics (`scheduler_process_metrics`)
- Average waiting and turnaround values (`scheduler_algorithm_summary`)

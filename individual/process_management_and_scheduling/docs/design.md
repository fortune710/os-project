# Subsystem A: Process Management & CPU Scheduling — Design Document

## Overview

This subsystem implements process lifecycle management and CPU scheduling for the MOSS
simulator. It provides the ability to create and terminate simulated processes, manage
their state through Process Control Blocks (PCBs), and execute two scheduling algorithms:
First-Come, First-Served (FCFS) and Round Robin (RR).

## Design Decisions

### Process Control Block

Each process is represented by a PCB containing: PID, name, state (NEW/READY/RUNNING/
WAITING/TERMINATED), priority, arrival time, burst time, remaining time, and computed
statistics (waiting time, turnaround time, completion time). The process table is a
fixed-size array of 32 slots with an `active` flag to track which slots are in use.

I chose static allocation over dynamic allocation to avoid memory management complexity.
PIDs are assigned sequentially using a monotonically increasing counter, guaranteeing
uniqueness even after processes are terminated.

### FCFS Algorithm

FCFS sorts processes by arrival time and runs each to completion. Ties are broken by PID.
If no process has arrived yet, the scheduler advances time and records an idle gap in the
Gantt chart. The algorithm is O(n log n) for the sort and O(n) for execution.

### Round Robin Algorithm

RR uses a queue of process indices. Each process runs for at most `quantum` time units
before being preempted. After each quantum, newly arrived processes are enqueued before
the preempted process — this ensures fair treatment of new arrivals while maintaining
the FIFO property of the ready queue.

### Gantt Chart

I implemented the Gantt chart as an array of `(pid, start_time, end_time)` tuples. The
chart is rendered as a text-based table with proportionally sized cells. Idle periods
are shown as `idle` entries.

## Assumptions

- All burst times and arrival times are non-negative integers.
- Process names are limited to 31 characters.
- Maximum 32 concurrent processes.
- Scheduling runs to completion (batch simulation, not interactive stepping).

## Limitations

- No preemptive priority scheduling.
- No multilevel feedback queue.
- No support for I/O bursts or process blocking during execution.

## API Summary

| Function | Purpose |
|----------|---------|
| `sched_init()` | Initialize the scheduler |
| `sched_create_process()` | Create a new process |
| `sched_terminate_process()` | Remove a process |
| `sched_run_fcfs()` | Execute FCFS scheduling |
| `sched_run_rr()` | Execute Round Robin scheduling |
| `sched_print_gantt()` | Display Gantt chart |
| `sched_print_stats()` | Display statistics |
| `sched_cleanup()` | Free all resources |

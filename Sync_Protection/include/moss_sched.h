/*
 * sched.h - Process Management & CPU Scheduling API
 * Subsystem A of MOSS
 *
 * Provides process creation/termination, ready queue management,
 * and CPU scheduling algorithms (FCFS, Round Robin).
 */

#ifndef MOSS_SCHED_H
#define MOSS_SCHED_H

#include "moss_common.h"

/*
 * sched_init - Initialize the scheduler subsystem.
 * Must be called before any other sched_ functions.
 * Returns: MOSS_SUCCESS on success.
 */
int sched_init(void);

/*
 * sched_create_process - Create a new process.
 * @name:     Process name (max MAX_NAME_LEN chars)
 * @burst:    CPU burst time (positive integer)
 * @arrival:  Arrival time (non-negative integer)
 * @priority: Priority value (lower = higher priority)
 * @role:     User role for access control
 * Returns: PID (>= 0) on success, negative error code on failure.
 *          MOSS_ERR_FULL if process table is full.
 *          MOSS_ERR_INVALID if parameters are invalid.
 */
int sched_create_process(const char *name, int burst, int arrival,
                         int priority, UserRole role);

/*
 * sched_terminate_process - Terminate a process by PID.
 * @pid: Process ID to terminate.
 * Returns: MOSS_SUCCESS on success, MOSS_ERR_NOT_FOUND if PID invalid.
 */
int sched_terminate_process(int pid);

/*
 * sched_run_fcfs - Execute FCFS scheduling on the ready queue.
 * Processes are scheduled in arrival order (non-preemptive).
 * Returns: MOSS_SUCCESS on success, MOSS_ERR_INVALID if no processes.
 */
int sched_run_fcfs(void);

/*
 * sched_run_rr - Execute Round Robin scheduling.
 * @quantum: Time quantum for each process.
 * Returns: MOSS_SUCCESS on success, MOSS_ERR_INVALID if quantum <= 0.
 */
int sched_run_rr(int quantum);

/*
 * sched_get_running - Get the PID of the currently running process.
 * Returns: PID (>= 0) or MOSS_ERR_NOT_FOUND if no process running.
 */
int sched_get_running(void);

/*
 * sched_get_process - Get a pointer to a process's PCB.
 * @pid: Process ID.
 * Returns: Pointer to PCB, or NULL if not found.
 */
PCB* sched_get_process(int pid);

/*
 * sched_print_gantt - Print the Gantt chart of the last scheduling run.
 * Output is sent to stdout as a text-based timeline.
 */
void sched_print_gantt(void);

/*
 * sched_print_stats - Print scheduling statistics.
 * Includes per-process waiting time, turnaround time, and averages.
 */
void sched_print_stats(void);

/*
 * sched_list_processes - Print a table of all processes and their states.
 * Returns: MOSS_SUCCESS.
 */
int sched_list_processes(void);

/*
 * sched_cleanup - Free all scheduler resources.
 * Should be called during system shutdown.
 */
void sched_cleanup(void);

#endif /* MOSS_SCHED_H */

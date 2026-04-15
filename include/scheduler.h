#ifndef MOSS_SCHEDULER_H
#define MOSS_SCHEDULER_H

#include "moss.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCHEDULER_ALGORITHM_FCFS = 0,
    SCHEDULER_ALGORITHM_ROUND_ROBIN = 1,
    SCHEDULER_ALGORITHM_PRIORITY = 2,
    SCHEDULER_ALGORITHM_MLFQ = 3
} scheduler_algorithm;

typedef enum {
    PROCESS_LIFECYCLE_NEW = 0,
    PROCESS_LIFECYCLE_READY = 1,
    PROCESS_LIFECYCLE_RUNNING = 2,
    PROCESS_LIFECYCLE_WAITING = 3,
    PROCESS_LIFECYCLE_TERMINATED = 4
} process_lifecycle;

typedef struct {
    moss_pid_t process_identifier;
    moss_time_t process_arrival_time;
    moss_time_t process_burst_time;
    moss_time_t process_remaining_time;
    moss_time_t process_waiting_time;
    moss_time_t process_turnaround_time;
    process_lifecycle process_lifecycle_state;
} process_control_block;

typedef struct {
    moss_pid_t process_identifier;
    moss_time_t process_arrival_time;
    moss_time_t process_burst_time;
    moss_time_t process_remaining_time;
    moss_time_t process_waiting_time;
    moss_time_t process_turnaround_time;
} scheduler_process;

typedef struct {
    moss_time_t current_time;
    moss_time_t total_waiting_time;
    moss_time_t total_turnaround_time;
    size_t completed_count;
} scheduler_statistics;

moss_status_t scheduler_init(scheduler_algorithm scheduling_algorithm, moss_time_t algorithm_time_quantum);
moss_status_t scheduler_create_process(const scheduler_process *scheduler_process_definition);
moss_status_t scheduler_terminate_process(moss_pid_t process_identifier);
moss_status_t scheduler_tick(moss_time_t time_delta);
moss_status_t scheduler_schedule(moss_pid_t *scheduled_process_identifier);
moss_status_t scheduler_get_statistics(scheduler_statistics *output_statistics);
moss_status_t scheduler_set_algorithm(scheduler_algorithm scheduling_algorithm, moss_time_t algorithm_time_quantum);

#ifdef __cplusplus
}
#endif

#endif

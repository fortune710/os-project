#ifndef MOSS_SCHED_H
#define MOSS_SCHED_H

#include "moss.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCHED_FCFS = 0,
    SCHED_RR = 1,
    SCHED_PRIORITY = 2,
    SCHED_MLFQ = 3
} sched_policy_t;

typedef struct {
    moss_pid_t pid;
    moss_time_t arrival_time;
    moss_time_t burst_time;
    moss_time_t remaining_time;
    moss_time_t waiting_time;
    moss_time_t turnaround_time;
} sched_process_t;

typedef struct {
    moss_time_t now;
    moss_time_t total_waiting_time;
    moss_time_t total_turnaround_time;
    size_t completed_count;
} sched_stats_t;

moss_status_t sched_init(sched_policy_t policy, moss_time_t quantum);
moss_status_t sched_create_process(const sched_process_t *proc);
moss_status_t sched_terminate_process(moss_pid_t pid);
moss_status_t sched_tick(moss_time_t delta);

moss_status_t sched_schedule(moss_pid_t *scheduled_pid);
moss_status_t sched_get_stats(sched_stats_t *out_stats);
moss_status_t sched_set_policy(sched_policy_t policy, moss_time_t quantum);

#ifdef __cplusplus
}
#endif

#endif

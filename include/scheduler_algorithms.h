#ifndef MOSS_SCHEDULER_ALGORITHMS_H
#define MOSS_SCHEDULER_ALGORITHMS_H

#include "scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    moss_pid_t process_identifier;
    moss_time_t start_time;
    moss_time_t end_time;
    int is_idle_segment;
} scheduler_gantt_segment;

typedef struct {
    moss_pid_t process_identifier;
    moss_time_t waiting_time;
    moss_time_t turnaround_time;
    moss_time_t completion_time;
} scheduler_process_metrics;

typedef struct {
    double average_waiting_time;
    double average_turnaround_time;
    size_t process_count;
} scheduler_algorithm_summary;

moss_status_t scheduler_run_fcfs(
    const scheduler_process *scheduler_process_definitions,
    size_t scheduler_process_count,
    scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segments_capacity,
    size_t *output_gantt_segment_count,
    scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_capacity,
    size_t *output_process_metrics_count,
    scheduler_algorithm_summary *output_algorithm_summary);

moss_status_t scheduler_run_round_robin(
    const scheduler_process *scheduler_process_definitions,
    size_t scheduler_process_count,
    moss_time_t algorithm_time_quantum,
    scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segments_capacity,
    size_t *output_gantt_segment_count,
    scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_capacity,
    size_t *output_process_metrics_count,
    scheduler_algorithm_summary *output_algorithm_summary);

moss_status_t scheduler_run_mlfq(
    const scheduler_process *scheduler_process_definitions,
    size_t scheduler_process_count,
    scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segments_capacity,
    size_t *output_gantt_segment_count,
    scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_capacity,
    size_t *output_process_metrics_count,
    scheduler_algorithm_summary *output_algorithm_summary);

#ifdef __cplusplus
}
#endif

#endif

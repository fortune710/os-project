#ifndef MOSS_SCHEDULER_ALGORITHMS_INTERNAL_HPP
#define MOSS_SCHEDULER_ALGORITHMS_INTERNAL_HPP

#include <deque>

#include "scheduler_algorithms.h"

moss_status_t scheduler_run_fcfs_with_ready_queue_pointer(
    const scheduler_process *scheduler_process_definitions,
    size_t scheduler_process_count,
    std::deque<moss_pid_t> *ready_queue_process_identifiers_pointer,
    scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segments_capacity,
    size_t *output_gantt_segment_count,
    scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_capacity,
    size_t *output_process_metrics_count,
    scheduler_algorithm_summary *output_algorithm_summary);

moss_status_t scheduler_run_round_robin_with_ready_queue_pointer(
    const scheduler_process *scheduler_process_definitions,
    size_t scheduler_process_count,
    moss_time_t algorithm_time_quantum,
    std::deque<moss_pid_t> *ready_queue_process_identifiers_pointer,
    scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segments_capacity,
    size_t *output_gantt_segment_count,
    scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_capacity,
    size_t *output_process_metrics_count,
    scheduler_algorithm_summary *output_algorithm_summary);

moss_status_t scheduler_run_mlfq_with_ready_queue_pointer(
    const scheduler_process *scheduler_process_definitions,
    size_t scheduler_process_count,
    std::deque<moss_pid_t> *ready_queue_process_identifiers_pointer,
    scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segments_capacity,
    size_t *output_gantt_segment_count,
    scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_capacity,
    size_t *output_process_metrics_count,
    scheduler_algorithm_summary *output_algorithm_summary);

#endif

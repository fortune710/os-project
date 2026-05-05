#include <stdio.h>

#include "scheduler_algorithms.h"
#include "mem.h"
#include "sync.h"

/* Print a Gantt chart style line with idle slices and process execution slices for one algorithm run. */
static void print_gantt_chart(
    const scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segment_count) {
    printf("Gantt Chart: ");
    for (size_t gantt_segment_index = 0; gantt_segment_index < output_gantt_segment_count; ++gantt_segment_index) {
        if (output_gantt_segments[gantt_segment_index].is_idle_segment != 0) {
            printf("[Idle %lld-%lld] ", (long long)output_gantt_segments[gantt_segment_index].start_time,
                   (long long)output_gantt_segments[gantt_segment_index].end_time);
            continue;
        }
        printf("[P%d %lld-%lld] ", output_gantt_segments[gantt_segment_index].process_identifier,
               (long long)output_gantt_segments[gantt_segment_index].start_time,
               (long long)output_gantt_segments[gantt_segment_index].end_time);
    }
    printf("\n");
}

/* Print per-process waiting and turnaround metrics plus average values for one algorithm execution. */
static void print_metrics(
    const scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_count,
    const scheduler_algorithm_summary *output_algorithm_summary) {
    printf("Process Metrics:\n");
    for (size_t process_metric_index = 0; process_metric_index < output_process_metrics_count; ++process_metric_index) {
        printf("  P%d -> Waiting: %lld, Turnaround: %lld, Completion: %lld\n",
               output_process_metrics[process_metric_index].process_identifier,
               (long long)output_process_metrics[process_metric_index].waiting_time,
               (long long)output_process_metrics[process_metric_index].turnaround_time,
               (long long)output_process_metrics[process_metric_index].completion_time);
    }
    printf("Average Waiting Time: %.2f\n", output_algorithm_summary->average_waiting_time);
    printf("Average Turnaround Time: %.2f\n", output_algorithm_summary->average_turnaround_time);
}

/* Run one algorithm function, print status, and show Gantt plus timing metrics when execution succeeds. */
static void run_and_print_algorithm(
    const char *algorithm_title,
    moss_status_t algorithm_status,
    const scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segment_count,
    const scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_count,
    const scheduler_algorithm_summary *output_algorithm_summary) {
    printf("\n=== %s ===\n", algorithm_title);
    if (algorithm_status != MOSS_OK) {
        printf("Algorithm failed with status code %d\n", algorithm_status);
        return;
    }
    print_gantt_chart(output_gantt_segments, output_gantt_segment_count);
    print_metrics(output_process_metrics, output_process_metrics_count, output_algorithm_summary);
}

/* Execute FCFS, Round Robin, and MLFQ demo runs and print terminal-friendly scheduling reports. */
int main(void) {
    scheduler_process scheduler_process_definitions[] = {
        {1, 0, 6, 0, 0, 0},
        {2, 0, 4, 0, 0, 0},
        {3, 2, 5, 0, 0, 0},
        {4, 9, 3, 0, 0, 0}};
    size_t scheduler_process_count = sizeof(scheduler_process_definitions) / sizeof(scheduler_process_definitions[0]);

    scheduler_gantt_segment output_gantt_segments[128];
    scheduler_process_metrics output_process_metrics[32];
    scheduler_algorithm_summary output_algorithm_summary = {0.0, 0.0, 0};
    size_t output_gantt_segment_count = 0;
    size_t output_process_metrics_count = 0;

    moss_status_t fcfs_status = scheduler_run_fcfs(
        scheduler_process_definitions, scheduler_process_count, output_gantt_segments,
        sizeof(output_gantt_segments) / sizeof(output_gantt_segments[0]), &output_gantt_segment_count,
        output_process_metrics, sizeof(output_process_metrics) / sizeof(output_process_metrics[0]),
        &output_process_metrics_count, &output_algorithm_summary);
    run_and_print_algorithm("FCFS", fcfs_status, output_gantt_segments, output_gantt_segment_count, output_process_metrics,
                            output_process_metrics_count, &output_algorithm_summary);

    output_gantt_segment_count = 0;
    output_process_metrics_count = 0;
    output_algorithm_summary.average_waiting_time = 0.0;
    output_algorithm_summary.average_turnaround_time = 0.0;
    output_algorithm_summary.process_count = 0;
    moss_status_t round_robin_status = scheduler_run_round_robin(
        scheduler_process_definitions, scheduler_process_count, 2, output_gantt_segments,
        sizeof(output_gantt_segments) / sizeof(output_gantt_segments[0]), &output_gantt_segment_count,
        output_process_metrics, sizeof(output_process_metrics) / sizeof(output_process_metrics[0]),
        &output_process_metrics_count, &output_algorithm_summary);
    run_and_print_algorithm("Round Robin (Quantum = 2)", round_robin_status, output_gantt_segments,
                            output_gantt_segment_count, output_process_metrics, output_process_metrics_count,
                            &output_algorithm_summary);

    output_gantt_segment_count = 0;
    output_process_metrics_count = 0;
    output_algorithm_summary.average_waiting_time = 0.0;
    output_algorithm_summary.average_turnaround_time = 0.0;
    output_algorithm_summary.process_count = 0;
    moss_status_t mlfq_status = scheduler_run_mlfq(
        scheduler_process_definitions, scheduler_process_count, output_gantt_segments,
        sizeof(output_gantt_segments) / sizeof(output_gantt_segments[0]), &output_gantt_segment_count,
        output_process_metrics, sizeof(output_process_metrics) / sizeof(output_process_metrics[0]),
        &output_process_metrics_count, &output_algorithm_summary);
    run_and_print_algorithm("MLFQ (Q0=1, Q1=2, Q2=FCFS)", mlfq_status, output_gantt_segments,
                            output_gantt_segment_count, output_process_metrics, output_process_metrics_count,
                            &output_algorithm_summary);

    return 0;
}

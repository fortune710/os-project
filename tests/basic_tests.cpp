#include <assert.h>
#include <math.h>

#include "scheduler.h"
#include "scheduler_algorithms.h"
#include "mem.h"
#include "sync.h"

/* Compare floating-point values with a small tolerance to keep average-metric checks stable. */
static int is_close(double left_value, double right_value) {
    return fabs(left_value - right_value) < 0.0001;
}

/* Validate FCFS behavior for tie ordering, metrics correctness, and deterministic Gantt output segments. */
static void run_fcfs_assertions(void) {
    scheduler_process scheduler_process_definitions[] = {{1, 0, 4, 0, 0, 0}, {2, 0, 3, 0, 0, 0}, {3, 5, 2, 0, 0, 0}};
    scheduler_gantt_segment output_gantt_segments[32];
    scheduler_process_metrics output_process_metrics[8];
    scheduler_algorithm_summary output_algorithm_summary = {0.0, 0.0, 0};
    size_t output_gantt_segment_count = 0;
    size_t output_process_metrics_count = 0;

    moss_status_t fcfs_status = scheduler_run_fcfs(
        scheduler_process_definitions, 3, output_gantt_segments, 32, &output_gantt_segment_count, output_process_metrics, 8,
        &output_process_metrics_count, &output_algorithm_summary);
    assert(fcfs_status == MOSS_OK);
    assert(output_gantt_segment_count == 3);
    assert(output_gantt_segments[0].process_identifier == 1);
    assert(output_gantt_segments[1].process_identifier == 2);
    assert(output_gantt_segments[2].process_identifier == 3);
    assert(output_process_metrics_count == 3);
    assert(output_process_metrics[0].waiting_time == 0);
    assert(output_process_metrics[1].waiting_time == 4);
    assert(output_process_metrics[2].waiting_time == 2);
    assert(output_process_metrics[0].turnaround_time == 4);
    assert(output_process_metrics[1].turnaround_time == 7);
    assert(output_process_metrics[2].turnaround_time == 4);
    assert(is_close(output_algorithm_summary.average_waiting_time, 2.0));
    assert(is_close(output_algorithm_summary.average_turnaround_time, 5.0));
}

/* Validate Round Robin quantum slicing, queue rotation, and computed averages for waiting and turnaround. */
static void run_round_robin_assertions(void) {
    scheduler_process scheduler_process_definitions[] = {{1, 0, 4, 0, 0, 0}, {2, 0, 3, 0, 0, 0}, {3, 5, 2, 0, 0, 0}};
    scheduler_gantt_segment output_gantt_segments[32];
    scheduler_process_metrics output_process_metrics[8];
    scheduler_algorithm_summary output_algorithm_summary = {0.0, 0.0, 0};
    size_t output_gantt_segment_count = 0;
    size_t output_process_metrics_count = 0;

    moss_status_t round_robin_status = scheduler_run_round_robin(
        scheduler_process_definitions, 3, 2, output_gantt_segments, 32, &output_gantt_segment_count, output_process_metrics,
        8, &output_process_metrics_count, &output_algorithm_summary);
    assert(round_robin_status == MOSS_OK);
    assert(output_gantt_segment_count == 5);
    assert(output_gantt_segments[0].process_identifier == 1);
    assert(output_gantt_segments[1].process_identifier == 2);
    assert(output_gantt_segments[2].process_identifier == 1);
    assert(output_gantt_segments[3].process_identifier == 2);
    assert(output_gantt_segments[4].process_identifier == 3);
    assert(output_process_metrics[0].waiting_time == 2);
    assert(output_process_metrics[1].waiting_time == 4);
    assert(output_process_metrics[2].waiting_time == 2);
    assert(is_close(output_algorithm_summary.average_waiting_time, 2.6666666667));
    assert(is_close(output_algorithm_summary.average_turnaround_time, 5.6666666667));
}

/* Validate MLFQ demotion flow across queues and resulting process timing metrics. */
static void run_mlfq_assertions(void) {
    scheduler_process scheduler_process_definitions[] = {{1, 0, 4, 0, 0, 0}, {2, 0, 3, 0, 0, 0}, {3, 5, 2, 0, 0, 0}};
    scheduler_gantt_segment output_gantt_segments[32];
    scheduler_process_metrics output_process_metrics[8];
    scheduler_algorithm_summary output_algorithm_summary = {0.0, 0.0, 0};
    size_t output_gantt_segment_count = 0;
    size_t output_process_metrics_count = 0;

    moss_status_t mlfq_status = scheduler_run_mlfq(
        scheduler_process_definitions, 3, output_gantt_segments, 32, &output_gantt_segment_count, output_process_metrics, 8,
        &output_process_metrics_count, &output_algorithm_summary);
    assert(mlfq_status == MOSS_OK);
    assert(output_gantt_segment_count == 6);
    assert(output_gantt_segments[0].process_identifier == 1);
    assert(output_gantt_segments[1].process_identifier == 2);
    assert(output_gantt_segments[2].process_identifier == 1);
    assert(output_gantt_segments[3].process_identifier == 2);
    assert(output_gantt_segments[4].process_identifier == 3);
    assert(output_gantt_segments[5].process_identifier == 1);
    assert(output_process_metrics[0].waiting_time == 5);
    assert(output_process_metrics[1].waiting_time == 3);
    assert(output_process_metrics[2].waiting_time == 1);
    assert(is_close(output_algorithm_summary.average_waiting_time, 3.0));
    assert(is_close(output_algorithm_summary.average_turnaround_time, 6.0));
}

/* Validate idle-gap handling and invalid-parameter edge cases across algorithm entry points. */
static void run_edge_case_assertions(void) {
    scheduler_process scheduler_process_definitions[] = {{9, 3, 2, 0, 0, 0}};
    scheduler_gantt_segment output_gantt_segments[16];
    scheduler_process_metrics output_process_metrics[4];
    scheduler_algorithm_summary output_algorithm_summary = {0.0, 0.0, 0};
    size_t output_gantt_segment_count = 0;
    size_t output_process_metrics_count = 0;

    moss_status_t idle_case_status = scheduler_run_fcfs(
        scheduler_process_definitions, 1, output_gantt_segments, 16, &output_gantt_segment_count, output_process_metrics, 4,
        &output_process_metrics_count, &output_algorithm_summary);
    assert(idle_case_status == MOSS_OK);
    assert(output_gantt_segment_count == 2);
    assert(output_gantt_segments[0].is_idle_segment != 0);
    assert(output_gantt_segments[0].start_time == 0);
    assert(output_gantt_segments[0].end_time == 3);
    assert(output_gantt_segments[1].process_identifier == 9);

    assert(scheduler_run_round_robin(
               scheduler_process_definitions, 1, 0, output_gantt_segments, 16, &output_gantt_segment_count,
               output_process_metrics, 4, &output_process_metrics_count, &output_algorithm_summary) == MOSS_EINVAL);
    assert(scheduler_run_fcfs(
               scheduler_process_definitions, 0, output_gantt_segments, 16, &output_gantt_segment_count,
               output_process_metrics, 4, &output_process_metrics_count, &output_algorithm_summary) == MOSS_EINVAL);
}

/* Execute baseline type checks and scheduler lifecycle assertions plus algorithm-specific correctness coverage. */
int main(void) {
    assert(MOSS_OK == 0);
    assert(MOSS_EINVAL < 0);
    assert(MOSS_ENOMEM < 0);
    assert(MOSS_ENOTREADY < 0);
    assert(MOSS_ESTATE < 0);

    (void)sizeof(scheduler_process);
    (void)sizeof(process_control_block);
    (void)sizeof(scheduler_gantt_segment);
    (void)sizeof(scheduler_process_metrics);
    (void)sizeof(mem_access_t);
    (void)sizeof(sync_access_t);

    assert(scheduler_create_process((const scheduler_process *)0) == MOSS_ENOTREADY);
    assert(scheduler_init(SCHEDULER_ALGORITHM_FCFS, 0) == MOSS_OK);
    assert(scheduler_init((scheduler_algorithm)99, 0) == MOSS_EINVAL);
    assert(scheduler_init(SCHEDULER_ALGORITHM_ROUND_ROBIN, 0) == MOSS_EINVAL);
    assert(scheduler_init(SCHEDULER_ALGORITHM_FCFS, 0) == MOSS_OK);

    scheduler_process first_scheduler_process_definition = {1, 0, 5, 0, 0, 0};
    scheduler_process second_scheduler_process_definition = {2, 0, 3, 0, 0, 0};
    assert(scheduler_create_process(&first_scheduler_process_definition) == MOSS_OK);
    assert(scheduler_create_process(&first_scheduler_process_definition) == MOSS_ESTATE);
    assert(scheduler_create_process(&second_scheduler_process_definition) == MOSS_OK);

    moss_pid_t scheduled_process_identifier = -1;
    assert(scheduler_schedule(&scheduled_process_identifier) == MOSS_OK);
    assert(scheduled_process_identifier == 1);
    assert(scheduler_tick(2) == MOSS_OK);

    scheduler_statistics scheduler_statistics_snapshot = {0, 0, 0, 0};
    assert(scheduler_get_statistics(&scheduler_statistics_snapshot) == MOSS_OK);
    assert(scheduler_statistics_snapshot.current_time == 2);
    assert(scheduler_statistics_snapshot.completed_count == 0);
    assert(scheduler_terminate_process(2) == MOSS_OK);
    assert(scheduler_terminate_process(99) == MOSS_EINVAL);
    assert(scheduler_tick(3) == MOSS_OK);
    assert(scheduler_get_statistics(&scheduler_statistics_snapshot) == MOSS_OK);
    assert(scheduler_statistics_snapshot.completed_count == 2);
    assert(scheduler_schedule(&scheduled_process_identifier) == MOSS_ESTATE);

    run_fcfs_assertions();
    run_round_robin_assertions();
    run_mlfq_assertions();
    run_edge_case_assertions();
    return 0;
}

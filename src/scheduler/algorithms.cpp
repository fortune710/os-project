#include "algorithms_internal.hpp"

#include <algorithm>
#include <deque>
#include <vector>

typedef struct {
    size_t process_input_index;
    moss_pid_t process_identifier;
    moss_time_t process_arrival_time;
    moss_time_t process_burst_time;
    moss_time_t process_remaining_time;
    moss_time_t process_completion_time;
    int process_is_completed;
} algorithm_runtime_process;

typedef struct {
    std::vector<size_t> arrival_ordered_process_indices;
    size_t next_arrival_order_index;
    moss_time_t simulation_time;
    size_t completed_process_count;
} algorithm_runtime_state;

/* Validate shared runner inputs, initialize output counters, and ensure capacities are safe for the requested process count. */
static moss_status_t validate_algorithm_inputs_and_prepare_outputs(
    const scheduler_process *scheduler_process_definitions,
    size_t scheduler_process_count,
    std::deque<moss_pid_t> *ready_queue_process_identifiers_pointer,
    scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segments_capacity,
    size_t *output_gantt_segment_count,
    scheduler_process_metrics *output_process_metrics,
    size_t output_process_metrics_capacity,
    size_t *output_process_metrics_count,
    scheduler_algorithm_summary *output_algorithm_summary) {
    if (scheduler_process_definitions == nullptr || ready_queue_process_identifiers_pointer == nullptr ||
        output_gantt_segments == nullptr || output_gantt_segment_count == nullptr ||
        output_process_metrics == nullptr || output_process_metrics_count == nullptr ||
        output_algorithm_summary == nullptr) {
        return MOSS_EINVAL;
    }
    if (scheduler_process_count == 0) {
        return MOSS_EINVAL;
    }
    if (output_process_metrics_capacity < scheduler_process_count) {
        return MOSS_ENOMEM;
    }
    *output_gantt_segment_count = 0;
    *output_process_metrics_count = 0;
    output_algorithm_summary->average_waiting_time = 0.0;
    output_algorithm_summary->average_turnaround_time = 0.0;
    output_algorithm_summary->process_count = scheduler_process_count;
    ready_queue_process_identifiers_pointer->clear();
    for (size_t process_input_index = 0; process_input_index < scheduler_process_count; ++process_input_index) {
        if (scheduler_process_definitions[process_input_index].process_burst_time <= 0 ||
            scheduler_process_definitions[process_input_index].process_arrival_time < 0) {
            return MOSS_EINVAL;
        }
    }
    if (output_gantt_segments_capacity == 0) {
        return MOSS_ENOMEM;
    }
    return MOSS_OK;
}

/* Build mutable runtime process state and deterministic arrival ordering with input-order tie behavior. */
static void initialize_runtime_state(
    const scheduler_process *scheduler_process_definitions,
    size_t scheduler_process_count,
    std::vector<algorithm_runtime_process> *runtime_processes,
    algorithm_runtime_state *runtime_state) {
    runtime_processes->clear();
    runtime_processes->reserve(scheduler_process_count);
    runtime_state->arrival_ordered_process_indices.clear();
    runtime_state->arrival_ordered_process_indices.reserve(scheduler_process_count);
    runtime_state->next_arrival_order_index = 0;
    runtime_state->simulation_time = 0;
    runtime_state->completed_process_count = 0;
    for (size_t process_input_index = 0; process_input_index < scheduler_process_count; ++process_input_index) {
        algorithm_runtime_process created_runtime_process = {};
        created_runtime_process.process_input_index = process_input_index;
        created_runtime_process.process_identifier = scheduler_process_definitions[process_input_index].process_identifier;
        created_runtime_process.process_arrival_time = scheduler_process_definitions[process_input_index].process_arrival_time;
        created_runtime_process.process_burst_time = scheduler_process_definitions[process_input_index].process_burst_time;
        created_runtime_process.process_remaining_time = scheduler_process_definitions[process_input_index].process_burst_time;
        created_runtime_process.process_completion_time = 0;
        created_runtime_process.process_is_completed = 0;
        runtime_processes->push_back(created_runtime_process);
        runtime_state->arrival_ordered_process_indices.push_back(process_input_index);
    }
    std::stable_sort(
        runtime_state->arrival_ordered_process_indices.begin(),
        runtime_state->arrival_ordered_process_indices.end(),
        [&](size_t left_process_index, size_t right_process_index) {
            return scheduler_process_definitions[left_process_index].process_arrival_time <
                   scheduler_process_definitions[right_process_index].process_arrival_time;
        });
}

/* Append a Gantt segment and merge adjacent segments that represent the same process or idle interval. */
static moss_status_t append_gantt_segment(
    scheduler_gantt_segment *output_gantt_segments,
    size_t output_gantt_segments_capacity,
    size_t *output_gantt_segment_count,
    moss_pid_t process_identifier,
    moss_time_t start_time,
    moss_time_t end_time,
    int is_idle_segment) {
    if (end_time <= start_time) {
        return MOSS_OK;
    }
    if (*output_gantt_segment_count > 0) {
        scheduler_gantt_segment *last_gantt_segment = &output_gantt_segments[*output_gantt_segment_count - 1];
        if (last_gantt_segment->process_identifier == process_identifier &&
            last_gantt_segment->is_idle_segment == is_idle_segment &&
            last_gantt_segment->end_time == start_time) {
            last_gantt_segment->end_time = end_time;
            return MOSS_OK;
        }
    }
    if (*output_gantt_segment_count >= output_gantt_segments_capacity) {
        return MOSS_ENOMEM;
    }
    output_gantt_segments[*output_gantt_segment_count].process_identifier = process_identifier;
    output_gantt_segments[*output_gantt_segment_count].start_time = start_time;
    output_gantt_segments[*output_gantt_segment_count].end_time = end_time;
    output_gantt_segments[*output_gantt_segment_count].is_idle_segment = is_idle_segment;
    *output_gantt_segment_count += 1;
    return MOSS_OK;
}

/* Move all newly arrived processes into the selected queue using deterministic arrival ordering. */
static void enqueue_new_arrivals(
    algorithm_runtime_state *runtime_state,
    const std::vector<algorithm_runtime_process> &runtime_processes,
    std::deque<moss_pid_t> *destination_ready_queue) {
    while (runtime_state->next_arrival_order_index < runtime_state->arrival_ordered_process_indices.size()) {
        size_t next_process_index = runtime_state->arrival_ordered_process_indices[runtime_state->next_arrival_order_index];
        if (runtime_processes[next_process_index].process_arrival_time > runtime_state->simulation_time) {
            break;
        }
        destination_ready_queue->push_back(runtime_processes[next_process_index].process_identifier);
        runtime_state->next_arrival_order_index += 1;
    }
}

/* Return the next process arrival time for idle-gap handling when the ready queue is empty. */
static moss_time_t next_arrival_time_or_current_time(
    const algorithm_runtime_state &runtime_state,
    const std::vector<algorithm_runtime_process> &runtime_processes) {
    if (runtime_state.next_arrival_order_index >= runtime_state.arrival_ordered_process_indices.size()) {
        return runtime_state.simulation_time;
    }
    size_t next_process_index = runtime_state.arrival_ordered_process_indices[runtime_state.next_arrival_order_index];
    return runtime_processes[next_process_index].process_arrival_time;
}

/* Locate runtime process array index from process identifier for queue-driven execution steps. */
static size_t find_runtime_process_index_by_identifier(
    const std::vector<algorithm_runtime_process> &runtime_processes,
    moss_pid_t process_identifier) {
    for (size_t process_index = 0; process_index < runtime_processes.size(); ++process_index) {
        if (runtime_processes[process_index].process_identifier == process_identifier) {
            return process_index;
        }
    }
    return runtime_processes.size();
}

/* Finalize waiting/turnaround per process and compute average metrics after scheduling completes. */
static void finalize_metrics(
    const std::vector<algorithm_runtime_process> &runtime_processes,
    scheduler_process_metrics *output_process_metrics,
    size_t *output_process_metrics_count,
    scheduler_algorithm_summary *output_algorithm_summary) {
    moss_time_t total_waiting_time = 0;
    moss_time_t total_turnaround_time = 0;
    *output_process_metrics_count = runtime_processes.size();
    for (size_t process_index = 0; process_index < runtime_processes.size(); ++process_index) {
        moss_time_t process_turnaround_time =
            runtime_processes[process_index].process_completion_time - runtime_processes[process_index].process_arrival_time;
        moss_time_t process_waiting_time = process_turnaround_time - runtime_processes[process_index].process_burst_time;
        output_process_metrics[process_index].process_identifier = runtime_processes[process_index].process_identifier;
        output_process_metrics[process_index].waiting_time = process_waiting_time;
        output_process_metrics[process_index].turnaround_time = process_turnaround_time;
        output_process_metrics[process_index].completion_time = runtime_processes[process_index].process_completion_time;
        total_waiting_time += process_waiting_time;
        total_turnaround_time += process_turnaround_time;
    }
    output_algorithm_summary->average_waiting_time =
        static_cast<double>(total_waiting_time) / static_cast<double>(runtime_processes.size());
    output_algorithm_summary->average_turnaround_time =
        static_cast<double>(total_turnaround_time) / static_cast<double>(runtime_processes.size());
}

/* Run FCFS scheduling with idle handling and deterministic same-arrival ordering while writing Gantt and metrics outputs. */
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
    scheduler_algorithm_summary *output_algorithm_summary) {
    moss_status_t validation_status = validate_algorithm_inputs_and_prepare_outputs(
        scheduler_process_definitions, scheduler_process_count, ready_queue_process_identifiers_pointer, output_gantt_segments,
        output_gantt_segments_capacity, output_gantt_segment_count, output_process_metrics, output_process_metrics_capacity,
        output_process_metrics_count, output_algorithm_summary);
    if (validation_status != MOSS_OK) {
        return validation_status;
    }

    std::vector<algorithm_runtime_process> runtime_processes;
    algorithm_runtime_state runtime_state = {};
    initialize_runtime_state(scheduler_process_definitions, scheduler_process_count, &runtime_processes, &runtime_state);

    while (runtime_state.completed_process_count < scheduler_process_count) {
        enqueue_new_arrivals(&runtime_state, runtime_processes, ready_queue_process_identifiers_pointer);
        if (ready_queue_process_identifiers_pointer->empty()) {
            moss_time_t next_process_arrival_time = next_arrival_time_or_current_time(runtime_state, runtime_processes);
            if (next_process_arrival_time > runtime_state.simulation_time) {
                moss_status_t append_status = append_gantt_segment(
                    output_gantt_segments, output_gantt_segments_capacity, output_gantt_segment_count, -1,
                    runtime_state.simulation_time, next_process_arrival_time, 1);
                if (append_status != MOSS_OK) {
                    return append_status;
                }
                runtime_state.simulation_time = next_process_arrival_time;
                continue;
            }
            return MOSS_ESTATE;
        }

        moss_pid_t selected_process_identifier = ready_queue_process_identifiers_pointer->front();
        ready_queue_process_identifiers_pointer->pop_front();
        size_t selected_runtime_process_index =
            find_runtime_process_index_by_identifier(runtime_processes, selected_process_identifier);
        if (selected_runtime_process_index >= runtime_processes.size()) {
            continue;
        }

        moss_time_t process_start_time = runtime_state.simulation_time;
        moss_time_t process_end_time = process_start_time + runtime_processes[selected_runtime_process_index].process_remaining_time;
        moss_status_t append_status = append_gantt_segment(
            output_gantt_segments, output_gantt_segments_capacity, output_gantt_segment_count, selected_process_identifier,
            process_start_time, process_end_time, 0);
        if (append_status != MOSS_OK) {
            return append_status;
        }
        runtime_state.simulation_time = process_end_time;
        runtime_processes[selected_runtime_process_index].process_remaining_time = 0;
        runtime_processes[selected_runtime_process_index].process_completion_time = process_end_time;
        runtime_processes[selected_runtime_process_index].process_is_completed = 1;
        runtime_state.completed_process_count += 1;
    }

    finalize_metrics(runtime_processes, output_process_metrics, output_process_metrics_count, output_algorithm_summary);
    return MOSS_OK;
}

/* Run Round Robin scheduling with caller-provided quantum and queue rotation semantics. */
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
    scheduler_algorithm_summary *output_algorithm_summary) {
    moss_status_t validation_status = validate_algorithm_inputs_and_prepare_outputs(
        scheduler_process_definitions, scheduler_process_count, ready_queue_process_identifiers_pointer, output_gantt_segments,
        output_gantt_segments_capacity, output_gantt_segment_count, output_process_metrics, output_process_metrics_capacity,
        output_process_metrics_count, output_algorithm_summary);
    if (validation_status != MOSS_OK) {
        return validation_status;
    }
    if (algorithm_time_quantum <= 0) {
        return MOSS_EINVAL;
    }

    std::vector<algorithm_runtime_process> runtime_processes;
    algorithm_runtime_state runtime_state = {};
    initialize_runtime_state(scheduler_process_definitions, scheduler_process_count, &runtime_processes, &runtime_state);

    while (runtime_state.completed_process_count < scheduler_process_count) {
        enqueue_new_arrivals(&runtime_state, runtime_processes, ready_queue_process_identifiers_pointer);
        if (ready_queue_process_identifiers_pointer->empty()) {
            moss_time_t next_process_arrival_time = next_arrival_time_or_current_time(runtime_state, runtime_processes);
            if (next_process_arrival_time > runtime_state.simulation_time) {
                moss_status_t append_status = append_gantt_segment(
                    output_gantt_segments, output_gantt_segments_capacity, output_gantt_segment_count, -1,
                    runtime_state.simulation_time, next_process_arrival_time, 1);
                if (append_status != MOSS_OK) {
                    return append_status;
                }
                runtime_state.simulation_time = next_process_arrival_time;
                continue;
            }
            return MOSS_ESTATE;
        }

        moss_pid_t selected_process_identifier = ready_queue_process_identifiers_pointer->front();
        ready_queue_process_identifiers_pointer->pop_front();
        size_t selected_runtime_process_index =
            find_runtime_process_index_by_identifier(runtime_processes, selected_process_identifier);
        if (selected_runtime_process_index >= runtime_processes.size()) {
            continue;
        }
        if (runtime_processes[selected_runtime_process_index].process_is_completed != 0 ||
            runtime_processes[selected_runtime_process_index].process_remaining_time <= 0) {
            continue;
        }

        moss_time_t execution_time_slice = algorithm_time_quantum;
        if (execution_time_slice > runtime_processes[selected_runtime_process_index].process_remaining_time) {
            execution_time_slice = runtime_processes[selected_runtime_process_index].process_remaining_time;
        }
        moss_time_t process_start_time = runtime_state.simulation_time;
        moss_time_t process_end_time = process_start_time + execution_time_slice;
        moss_status_t append_status = append_gantt_segment(
            output_gantt_segments, output_gantt_segments_capacity, output_gantt_segment_count, selected_process_identifier,
            process_start_time, process_end_time, 0);
        if (append_status != MOSS_OK) {
            return append_status;
        }
        runtime_state.simulation_time = process_end_time;
        runtime_processes[selected_runtime_process_index].process_remaining_time -= execution_time_slice;
        enqueue_new_arrivals(&runtime_state, runtime_processes, ready_queue_process_identifiers_pointer);

        if (runtime_processes[selected_runtime_process_index].process_remaining_time > 0) {
            ready_queue_process_identifiers_pointer->push_back(selected_process_identifier);
            continue;
        }

        runtime_processes[selected_runtime_process_index].process_completion_time = runtime_state.simulation_time;
        runtime_processes[selected_runtime_process_index].process_is_completed = 1;
        runtime_state.completed_process_count += 1;
    }

    finalize_metrics(runtime_processes, output_process_metrics, output_process_metrics_count, output_algorithm_summary);
    return MOSS_OK;
}

/* Run fixed three-level MLFQ scheduling with RR/RR/FCFS behavior and demotion on exhausted time slices. */
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
    scheduler_algorithm_summary *output_algorithm_summary) {
    moss_status_t validation_status = validate_algorithm_inputs_and_prepare_outputs(
        scheduler_process_definitions, scheduler_process_count, ready_queue_process_identifiers_pointer, output_gantt_segments,
        output_gantt_segments_capacity, output_gantt_segment_count, output_process_metrics, output_process_metrics_capacity,
        output_process_metrics_count, output_algorithm_summary);
    if (validation_status != MOSS_OK) {
        return validation_status;
    }

    std::deque<moss_pid_t> middle_priority_ready_queue_process_identifiers;
    std::deque<moss_pid_t> low_priority_ready_queue_process_identifiers;
    const moss_time_t high_priority_time_quantum = 1;
    const moss_time_t middle_priority_time_quantum = 2;

    std::vector<algorithm_runtime_process> runtime_processes;
    algorithm_runtime_state runtime_state = {};
    initialize_runtime_state(scheduler_process_definitions, scheduler_process_count, &runtime_processes, &runtime_state);

    while (runtime_state.completed_process_count < scheduler_process_count) {
        enqueue_new_arrivals(&runtime_state, runtime_processes, ready_queue_process_identifiers_pointer);
        if (ready_queue_process_identifiers_pointer->empty() &&
            middle_priority_ready_queue_process_identifiers.empty() &&
            low_priority_ready_queue_process_identifiers.empty()) {
            moss_time_t next_process_arrival_time = next_arrival_time_or_current_time(runtime_state, runtime_processes);
            if (next_process_arrival_time > runtime_state.simulation_time) {
                moss_status_t append_status = append_gantt_segment(
                    output_gantt_segments, output_gantt_segments_capacity, output_gantt_segment_count, -1,
                    runtime_state.simulation_time, next_process_arrival_time, 1);
                if (append_status != MOSS_OK) {
                    return append_status;
                }
                runtime_state.simulation_time = next_process_arrival_time;
                continue;
            }
            return MOSS_ESTATE;
        }

        std::deque<moss_pid_t> *selected_ready_queue_pointer = nullptr;
        moss_time_t selected_time_quantum = 0;
        int selected_is_low_priority_queue = 0;
        if (!ready_queue_process_identifiers_pointer->empty()) {
            selected_ready_queue_pointer = ready_queue_process_identifiers_pointer;
            selected_time_quantum = high_priority_time_quantum;
        } else if (!middle_priority_ready_queue_process_identifiers.empty()) {
            selected_ready_queue_pointer = &middle_priority_ready_queue_process_identifiers;
            selected_time_quantum = middle_priority_time_quantum;
        } else {
            selected_ready_queue_pointer = &low_priority_ready_queue_process_identifiers;
            selected_is_low_priority_queue = 1;
        }

        moss_pid_t selected_process_identifier = selected_ready_queue_pointer->front();
        selected_ready_queue_pointer->pop_front();
        size_t selected_runtime_process_index =
            find_runtime_process_index_by_identifier(runtime_processes, selected_process_identifier);
        if (selected_runtime_process_index >= runtime_processes.size()) {
            continue;
        }
        if (runtime_processes[selected_runtime_process_index].process_is_completed != 0 ||
            runtime_processes[selected_runtime_process_index].process_remaining_time <= 0) {
            continue;
        }

        moss_time_t execution_time_slice = runtime_processes[selected_runtime_process_index].process_remaining_time;
        if (selected_is_low_priority_queue == 0 && execution_time_slice > selected_time_quantum) {
            execution_time_slice = selected_time_quantum;
        }
        moss_time_t process_start_time = runtime_state.simulation_time;
        moss_time_t process_end_time = process_start_time + execution_time_slice;
        moss_status_t append_status = append_gantt_segment(
            output_gantt_segments, output_gantt_segments_capacity, output_gantt_segment_count, selected_process_identifier,
            process_start_time, process_end_time, 0);
        if (append_status != MOSS_OK) {
            return append_status;
        }

        runtime_state.simulation_time = process_end_time;
        runtime_processes[selected_runtime_process_index].process_remaining_time -= execution_time_slice;
        enqueue_new_arrivals(&runtime_state, runtime_processes, ready_queue_process_identifiers_pointer);

        if (runtime_processes[selected_runtime_process_index].process_remaining_time <= 0) {
            runtime_processes[selected_runtime_process_index].process_completion_time = runtime_state.simulation_time;
            runtime_processes[selected_runtime_process_index].process_is_completed = 1;
            runtime_state.completed_process_count += 1;
            continue;
        }

        if (selected_ready_queue_pointer == ready_queue_process_identifiers_pointer) {
            middle_priority_ready_queue_process_identifiers.push_back(selected_process_identifier);
            continue;
        }
        low_priority_ready_queue_process_identifiers.push_back(selected_process_identifier);
    }

    finalize_metrics(runtime_processes, output_process_metrics, output_process_metrics_count, output_algorithm_summary);
    return MOSS_OK;
}

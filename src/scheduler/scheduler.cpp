#include "scheduler.h"

#include <algorithm>
#include <deque>
#include <unordered_map>

static bool scheduler_is_initialized = false;
static scheduler_algorithm active_scheduling_algorithm = SCHEDULER_ALGORITHM_FCFS;
static moss_time_t configured_algorithm_time_quantum = 0;
static std::unordered_map<moss_pid_t, process_control_block> pcb_by_identifier;
static std::deque<moss_pid_t> ready_queue_process_identifiers;
static bool has_current_running_process = false;
static moss_pid_t current_running_process_identifier = 0;
static scheduler_statistics scheduler_stats = {0, 0, 0, 0};

/* Reset scheduler state and validate the selected algorithm configuration. */
moss_status_t scheduler_init(scheduler_algorithm scheduling_algorithm, moss_time_t algorithm_time_quantum) {
    if (scheduling_algorithm != SCHEDULER_ALGORITHM_FCFS &&
        scheduling_algorithm != SCHEDULER_ALGORITHM_ROUND_ROBIN &&
        scheduling_algorithm != SCHEDULER_ALGORITHM_PRIORITY &&
        scheduling_algorithm != SCHEDULER_ALGORITHM_MLFQ) {
        return MOSS_EINVAL;
    }
    if (scheduling_algorithm == SCHEDULER_ALGORITHM_ROUND_ROBIN && algorithm_time_quantum <= 0) {
        return MOSS_EINVAL;
    }

    active_scheduling_algorithm = scheduling_algorithm;
    configured_algorithm_time_quantum = algorithm_time_quantum;
    pcb_by_identifier.clear();
    ready_queue_process_identifiers.clear();
    has_current_running_process = false;
    current_running_process_identifier = 0;
    scheduler_stats.current_time = 0;
    scheduler_stats.total_waiting_time = 0;
    scheduler_stats.total_turnaround_time = 0;
    scheduler_stats.completed_count = 0;
    scheduler_is_initialized = true;
    return MOSS_OK;
}

/* Create a process entry, apply NEW to READY transition, and enqueue it for scheduling. */
moss_status_t scheduler_create_process(const scheduler_process *scheduler_process_definition) {
    if (!scheduler_is_initialized) {
        return MOSS_ENOTREADY;
    }
    if (scheduler_process_definition == nullptr) {
        return MOSS_EINVAL;
    }
    if (scheduler_process_definition->process_burst_time <= 0) {
        return MOSS_EINVAL;
    }
    if (pcb_by_identifier.find(scheduler_process_definition->process_identifier) != pcb_by_identifier.end()) {
        return MOSS_ESTATE;
    }

    process_control_block created_process_control_block = {};
    created_process_control_block.process_identifier = scheduler_process_definition->process_identifier;
    created_process_control_block.process_arrival_time = scheduler_process_definition->process_arrival_time;
    created_process_control_block.process_burst_time = scheduler_process_definition->process_burst_time;
    created_process_control_block.process_remaining_time = scheduler_process_definition->process_burst_time;
    created_process_control_block.process_waiting_time = 0;
    created_process_control_block.process_turnaround_time = 0;
    created_process_control_block.process_lifecycle_state = PROCESS_LIFECYCLE_NEW;
    created_process_control_block.process_lifecycle_state = PROCESS_LIFECYCLE_READY;

    pcb_by_identifier[created_process_control_block.process_identifier] = created_process_control_block;
    ready_queue_process_identifiers.push_back(created_process_control_block.process_identifier);
    return MOSS_OK;
}

/* Terminate a process, aggregate statistics, and remove it from all active scheduler queues and maps. */
moss_status_t scheduler_terminate_process(moss_pid_t process_identifier) {
    if (!scheduler_is_initialized) {
        return MOSS_ENOTREADY;
    }

    auto process_control_block_iterator = pcb_by_identifier.find(process_identifier);
    if (process_control_block_iterator == pcb_by_identifier.end()) {
        return MOSS_EINVAL;
    }

    ready_queue_process_identifiers.erase(
        std::remove(ready_queue_process_identifiers.begin(), ready_queue_process_identifiers.end(), process_identifier),
        ready_queue_process_identifiers.end());

    if (has_current_running_process && current_running_process_identifier == process_identifier) {
        has_current_running_process = false;
        current_running_process_identifier = 0;
    }

    process_control_block_iterator->second.process_lifecycle_state = PROCESS_LIFECYCLE_TERMINATED;
    scheduler_stats.total_waiting_time += process_control_block_iterator->second.process_waiting_time;
    scheduler_stats.total_turnaround_time += process_control_block_iterator->second.process_turnaround_time;
    scheduler_stats.completed_count += 1;
    pcb_by_identifier.erase(process_control_block_iterator);
    return MOSS_OK;
}

/* Advance simulation time and update waiting, running, turnaround, and completion bookkeeping. */
moss_status_t scheduler_tick(moss_time_t time_delta) {
    if (!scheduler_is_initialized) {
        return MOSS_ENOTREADY;
    }
    if (time_delta < 0) {
        return MOSS_EINVAL;
    }

    scheduler_stats.current_time += time_delta;

    for (moss_pid_t ready_process_identifier : ready_queue_process_identifiers) {
        auto process_control_block_iterator = pcb_by_identifier.find(ready_process_identifier);
        if (process_control_block_iterator == pcb_by_identifier.end()) {
            continue;
        }
        process_control_block_iterator->second.process_waiting_time += time_delta;
        process_control_block_iterator->second.process_turnaround_time += time_delta;
        process_control_block_iterator->second.process_lifecycle_state = PROCESS_LIFECYCLE_READY;
    }

    if (!has_current_running_process) {
        return MOSS_OK;
    }

    auto running_process_control_block_iterator = pcb_by_identifier.find(current_running_process_identifier);
    if (running_process_control_block_iterator == pcb_by_identifier.end()) {
        has_current_running_process = false;
        current_running_process_identifier = 0;
        return MOSS_ESTATE;
    }

    running_process_control_block_iterator->second.process_lifecycle_state = PROCESS_LIFECYCLE_RUNNING;
    moss_time_t consumed_running_time = time_delta;
    if (consumed_running_time > running_process_control_block_iterator->second.process_remaining_time) {
        consumed_running_time = running_process_control_block_iterator->second.process_remaining_time;
    }

    running_process_control_block_iterator->second.process_turnaround_time += consumed_running_time;
    running_process_control_block_iterator->second.process_remaining_time -= consumed_running_time;

    if (running_process_control_block_iterator->second.process_remaining_time != 0) {
        return MOSS_OK;
    }

    moss_time_t finalized_waiting_time = running_process_control_block_iterator->second.process_waiting_time;
    moss_time_t finalized_turnaround_time = running_process_control_block_iterator->second.process_turnaround_time;
    running_process_control_block_iterator->second.process_lifecycle_state = PROCESS_LIFECYCLE_TERMINATED;
    pcb_by_identifier.erase(running_process_control_block_iterator);
    has_current_running_process = false;
    current_running_process_identifier = 0;
    scheduler_stats.total_waiting_time += finalized_waiting_time;
    scheduler_stats.total_turnaround_time += finalized_turnaround_time;
    scheduler_stats.completed_count += 1;
    return MOSS_OK;
}

/* Select the next runnable process according to algorithm behavior and update lifecycle state to RUNNING. */
moss_status_t scheduler_schedule(moss_pid_t *scheduled_process_identifier) {
    if (!scheduler_is_initialized) {
        return MOSS_ENOTREADY;
    }
    if (scheduled_process_identifier == nullptr) {
        return MOSS_EINVAL;
    }

    if (has_current_running_process && active_scheduling_algorithm == SCHEDULER_ALGORITHM_FCFS) {
        *scheduled_process_identifier = current_running_process_identifier;
        return MOSS_OK;
    }

    if (has_current_running_process && active_scheduling_algorithm == SCHEDULER_ALGORITHM_ROUND_ROBIN) {
        auto running_process_control_block_iterator = pcb_by_identifier.find(current_running_process_identifier);
        if (running_process_control_block_iterator != pcb_by_identifier.end()) {
            running_process_control_block_iterator->second.process_lifecycle_state = PROCESS_LIFECYCLE_READY;
            ready_queue_process_identifiers.push_back(current_running_process_identifier);
        }
        has_current_running_process = false;
        current_running_process_identifier = 0;
    }

    while (!ready_queue_process_identifiers.empty()) {
        moss_pid_t next_ready_process_identifier = ready_queue_process_identifiers.front();
        ready_queue_process_identifiers.pop_front();
        auto next_ready_process_control_block_iterator = pcb_by_identifier.find(next_ready_process_identifier);
        if (next_ready_process_control_block_iterator == pcb_by_identifier.end()) {
            continue;
        }
        if (next_ready_process_control_block_iterator->second.process_remaining_time <= 0) {
            continue;
        }
        next_ready_process_control_block_iterator->second.process_lifecycle_state = PROCESS_LIFECYCLE_RUNNING;
        current_running_process_identifier = next_ready_process_identifier;
        has_current_running_process = true;
        *scheduled_process_identifier = next_ready_process_identifier;
        return MOSS_OK;
    }

    return MOSS_ESTATE;
}

/* Copy current scheduler aggregate statistics into the output structure. */
moss_status_t scheduler_get_statistics(scheduler_statistics *output_statistics) {
    if (!scheduler_is_initialized) {
        return MOSS_ENOTREADY;
    }
    if (output_statistics == nullptr) {
        return MOSS_EINVAL;
    }
    *output_statistics = scheduler_stats;
    return MOSS_OK;
}

/* Update the active scheduling algorithm settings while keeping existing process state intact. */
moss_status_t scheduler_set_algorithm(scheduler_algorithm scheduling_algorithm, moss_time_t algorithm_time_quantum) {
    if (!scheduler_is_initialized) {
        return MOSS_ENOTREADY;
    }
    if (scheduling_algorithm != SCHEDULER_ALGORITHM_FCFS &&
        scheduling_algorithm != SCHEDULER_ALGORITHM_ROUND_ROBIN &&
        scheduling_algorithm != SCHEDULER_ALGORITHM_PRIORITY &&
        scheduling_algorithm != SCHEDULER_ALGORITHM_MLFQ) {
        return MOSS_EINVAL;
    }
    if (scheduling_algorithm == SCHEDULER_ALGORITHM_ROUND_ROBIN && algorithm_time_quantum <= 0) {
        return MOSS_EINVAL;
    }
    active_scheduling_algorithm = scheduling_algorithm;
    configured_algorithm_time_quantum = algorithm_time_quantum;
    return MOSS_OK;
}

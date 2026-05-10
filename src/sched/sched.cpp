/*
 * sched.cpp - Process Management & CPU Scheduling Implementation
 * Subsystem A of MOSS
 *
 * Implements process creation/termination, ready queue management,
 * FCFS and Round Robin scheduling, Gantt chart output, and statistics.
 */

#include "moss_sched.h"
#include <algorithm>
#include <vector>

/* ============================================================
 * Internal State
 * ============================================================ */

/* Process table: fixed-size array of PCBs */
static PCB process_table[MAX_PROCESSES];
static int process_count = 0;
static int next_pid = 0;
static int running_pid = -1;

/* Gantt chart storage */
static GanttEntry gantt_chart[MAX_GANTT];
static int gantt_count = 0;

/* Last scheduling stats */
static double avg_waiting_time = 0.0;
static double avg_turnaround_time = 0.0;

/* ============================================================
 * API Implementation
 * ============================================================ */

int sched_init(void) {
    std::memset(process_table, 0, sizeof(process_table));
    process_count = 0;
    next_pid = 0;
    running_pid = -1;
    gantt_count = 0;
    avg_waiting_time = 0.0;
    avg_turnaround_time = 0.0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].active = 0;
        process_table[i].pid = -1;
    }

    moss_log(LOG_INFO, "Scheduler subsystem initialized");
    return MOSS_SUCCESS;
}

int sched_create_process(const char *name, int burst, int arrival,
                         int priority, UserRole role) {
    if (name == nullptr || burst <= 0 || arrival < 0) {
        return MOSS_ERR_INVALID;
    }

    if (process_count >= MAX_PROCESSES) {
        return MOSS_ERR_FULL;
    }

    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        return MOSS_ERR_FULL;
    }

    PCB *p = &process_table[slot];
    p->pid = next_pid++;
    std::strncpy(p->name, name, MAX_NAME_LEN - 1);
    p->name[MAX_NAME_LEN - 1] = '\0';
    p->state = PROC_READY;
    p->priority = priority;
    p->arrival_time = arrival;
    p->burst_time = burst;
    p->remaining_time = burst;
    p->waiting_time = 0;
    p->turnaround_time = 0;
    p->completion_time = 0;
    p->role = role;
    p->active = 1;
    process_count++;

    moss_log(LOG_INFO, "Process created: PID=%d Name=%s Burst=%d Arrival=%d Role=%s",
             p->pid, p->name, burst, arrival, role_str(role));

    return p->pid;
}

int sched_terminate_process(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].active && process_table[i].pid == pid) {
            process_table[i].state = PROC_TERMINATED;
            process_table[i].active = 0;
            process_count--;
            if (running_pid == pid) {
                running_pid = -1;
            }
            moss_log(LOG_INFO, "Process terminated: PID=%d", pid);
            return MOSS_SUCCESS;
        }
    }
    return MOSS_ERR_NOT_FOUND;
}

int sched_run_fcfs(void) {
    /* Collect ready processes into a vector */
    std::vector<int> ready;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].active &&
            process_table[i].state != PROC_TERMINATED) {
            ready.push_back(i);
        }
    }

    if (ready.empty()) {
        return MOSS_ERR_INVALID;
    }

    /* Sort by arrival time, then PID for tie-breaking */
    std::sort(ready.begin(), ready.end(), [](int a, int b) {
        if (process_table[a].arrival_time != process_table[b].arrival_time)
            return process_table[a].arrival_time < process_table[b].arrival_time;
        return process_table[a].pid < process_table[b].pid;
    });

    /* Reset Gantt chart */
    gantt_count = 0;
    int current_time = 0;

    /* Reset process stats */
    for (int idx : ready) {
        PCB *p = &process_table[idx];
        p->remaining_time = p->burst_time;
        p->state = PROC_READY;
    }

    /* FCFS: run each process to completion in arrival order */
    for (int idx : ready) {
        PCB *p = &process_table[idx];

        /* If process hasn't arrived yet, advance time */
        if (current_time < p->arrival_time) {
            /* Record idle time in Gantt chart */
            if (gantt_count < MAX_GANTT) {
                gantt_chart[gantt_count].pid = -1; /* idle */
                gantt_chart[gantt_count].start_time = current_time;
                gantt_chart[gantt_count].end_time = p->arrival_time;
                gantt_count++;
            }
            current_time = p->arrival_time;
        }

        p->state = PROC_RUNNING;
        running_pid = p->pid;

        /* Record in Gantt chart */
        if (gantt_count < MAX_GANTT) {
            gantt_chart[gantt_count].pid = p->pid;
            gantt_chart[gantt_count].start_time = current_time;
            gantt_chart[gantt_count].end_time = current_time + p->burst_time;
            gantt_count++;
        }

        current_time += p->burst_time;
        p->remaining_time = 0;
        p->completion_time = current_time;
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->burst_time;
        p->state = PROC_TERMINATED;
    }

    running_pid = -1;

    /* Calculate averages */
    double total_wait = 0, total_turn = 0;
    for (int idx : ready) {
        PCB *p = &process_table[idx];
        total_wait += p->waiting_time;
        total_turn += p->turnaround_time;
    }
    avg_waiting_time = total_wait / static_cast<int>(ready.size());
    avg_turnaround_time = total_turn / static_cast<int>(ready.size());

    moss_log(LOG_INFO, "FCFS scheduling completed for %d processes",
             static_cast<int>(ready.size()));
    return MOSS_SUCCESS;
}

int sched_run_rr(int quantum) {
    if (quantum <= 0) {
        return MOSS_ERR_INVALID;
    }

    /* Collect ready processes */
    std::vector<int> ready;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].active &&
            process_table[i].state != PROC_TERMINATED) {
            ready.push_back(i);
        }
    }

    if (ready.empty()) {
        return MOSS_ERR_INVALID;
    }

    /* Sort by arrival time */
    std::sort(ready.begin(), ready.end(), [](int a, int b) {
        if (process_table[a].arrival_time != process_table[b].arrival_time)
            return process_table[a].arrival_time < process_table[b].arrival_time;
        return process_table[a].pid < process_table[b].pid;
    });

    int ready_count = static_cast<int>(ready.size());

    /* Reset Gantt chart and process stats */
    gantt_count = 0;
    int current_time = 0;

    for (int idx : ready) {
        PCB *p = &process_table[idx];
        p->remaining_time = p->burst_time;
        p->waiting_time = 0;
        p->turnaround_time = 0;
        p->completion_time = 0;
        p->state = PROC_READY;
    }

    /* Round Robin scheduling using a queue */
    std::vector<int> queue; /* Queue of indices into ready[] */
    int q_front = 0;
    int completed = 0;
    std::vector<bool> arrived(ready_count, false);

    /* Enqueue processes that arrive at time 0 */
    for (int i = 0; i < ready_count; i++) {
        PCB *p = &process_table[ready[i]];
        if (p->arrival_time <= current_time) {
            queue.push_back(i);
            arrived[i] = true;
        }
    }

    while (completed < ready_count) {
        if (q_front >= static_cast<int>(queue.size())) {
            /* No process in queue - advance to next arrival */
            int min_arrival = 999999;
            for (int i = 0; i < ready_count; i++) {
                PCB *p = &process_table[ready[i]];
                if (p->remaining_time > 0 && !arrived[i] &&
                    p->arrival_time < min_arrival) {
                    min_arrival = p->arrival_time;
                }
            }
            if (min_arrival == 999999) break;

            /* Record idle time */
            if (gantt_count < MAX_GANTT) {
                gantt_chart[gantt_count].pid = -1;
                gantt_chart[gantt_count].start_time = current_time;
                gantt_chart[gantt_count].end_time = min_arrival;
                gantt_count++;
            }
            current_time = min_arrival;

            /* Enqueue newly arrived */
            for (int i = 0; i < ready_count; i++) {
                PCB *p = &process_table[ready[i]];
                if (!arrived[i] && p->arrival_time <= current_time) {
                    queue.push_back(i);
                    arrived[i] = true;
                }
            }
            continue;
        }

        int idx = queue[q_front++];
        PCB *p = &process_table[ready[idx]];

        if (p->remaining_time <= 0) {
            continue; /* Already done */
        }

        p->state = PROC_RUNNING;
        running_pid = p->pid;

        int exec_time = (p->remaining_time < quantum) ? p->remaining_time : quantum;

        /* Record in Gantt chart */
        if (gantt_count < MAX_GANTT) {
            gantt_chart[gantt_count].pid = p->pid;
            gantt_chart[gantt_count].start_time = current_time;
            gantt_chart[gantt_count].end_time = current_time + exec_time;
            gantt_count++;
        }

        current_time += exec_time;
        p->remaining_time -= exec_time;

        /* Enqueue any new arrivals that came during this quantum */
        for (int i = 0; i < ready_count; i++) {
            if (!arrived[i]) {
                PCB *np = &process_table[ready[i]];
                if (np->arrival_time <= current_time) {
                    queue.push_back(i);
                    arrived[i] = true;
                }
            }
        }

        if (p->remaining_time <= 0) {
            /* Process completed */
            p->completion_time = current_time;
            p->turnaround_time = p->completion_time - p->arrival_time;
            p->waiting_time = p->turnaround_time - p->burst_time;
            p->state = PROC_TERMINATED;
            completed++;
        } else {
            /* Re-enqueue */
            p->state = PROC_READY;
            queue.push_back(idx);
        }
    }

    running_pid = -1;

    /* Calculate averages */
    double total_wait = 0, total_turn = 0;
    for (int idx : ready) {
        PCB *p = &process_table[idx];
        total_wait += p->waiting_time;
        total_turn += p->turnaround_time;
    }
    avg_waiting_time = total_wait / ready_count;
    avg_turnaround_time = total_turn / ready_count;

    moss_log(LOG_INFO, "Round Robin (quantum=%d) scheduling completed for %d processes",
             quantum, ready_count);
    return MOSS_SUCCESS;
}

int sched_get_running(void) {
    return running_pid;
}

PCB* sched_get_process(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].active && process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return nullptr;
}

void sched_print_gantt(void) {
    if (gantt_count == 0) {
        std::printf("  No scheduling data available. Run a scheduling algorithm first.\n");
        return;
    }

    /* Print top border */
    std::printf("  Gantt Chart:\n  ");
    for (int i = 0; i < gantt_count; i++) {
        int width = (gantt_chart[i].end_time - gantt_chart[i].start_time);
        if (width < 1) width = 1;
        int cell_width = width * 3;
        if (cell_width < 5) cell_width = 5;

        std::printf("+");
        for (int j = 0; j < cell_width; j++) std::printf("-");
    }
    std::printf("+\n  ");

    /* Print process IDs */
    for (int i = 0; i < gantt_count; i++) {
        int width = (gantt_chart[i].end_time - gantt_chart[i].start_time);
        if (width < 1) width = 1;
        int cell_width = width * 3;
        if (cell_width < 5) cell_width = 5;

        if (gantt_chart[i].pid == -1) {
            std::printf("|%*s", cell_width, "idle ");
        } else {
            char label[16];
            std::snprintf(label, sizeof(label), " P%d ", gantt_chart[i].pid);
            int pad = cell_width - static_cast<int>(std::strlen(label));
            int left_pad = pad / 2;
            int right_pad = pad - left_pad;
            std::printf("|");
            for (int j = 0; j < left_pad; j++) std::printf(" ");
            std::printf("%s", label);
            for (int j = 0; j < right_pad; j++) std::printf(" ");
        }
    }
    std::printf("|\n  ");

    /* Print bottom border */
    for (int i = 0; i < gantt_count; i++) {
        int width = (gantt_chart[i].end_time - gantt_chart[i].start_time);
        if (width < 1) width = 1;
        int cell_width = width * 3;
        if (cell_width < 5) cell_width = 5;

        std::printf("+");
        for (int j = 0; j < cell_width; j++) std::printf("-");
    }
    std::printf("+\n  ");

    /* Print timeline */
    for (int i = 0; i < gantt_count; i++) {
        int width = (gantt_chart[i].end_time - gantt_chart[i].start_time);
        if (width < 1) width = 1;
        int cell_width = width * 3;
        if (cell_width < 5) cell_width = 5;

        std::printf("%-*d", cell_width + 1, gantt_chart[i].start_time);
    }
    std::printf("%d\n", gantt_chart[gantt_count - 1].end_time);
}

void sched_print_stats(void) {
    std::printf("  %-6s %-16s %-8s %-8s %-10s %-12s %-12s\n",
           "PID", "Name", "Burst", "Arrival", "Complete", "Waiting", "Turnaround");
    std::printf("  %-6s %-16s %-8s %-8s %-10s %-12s %-12s\n",
           "---", "----", "-----", "-------", "--------", "-------", "----------");

    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid >= 0 && process_table[i].burst_time > 0) {
            PCB *p = &process_table[i];
            std::printf("  %-6d %-16s %-8d %-8d %-10d %-12d %-12d\n",
                   p->pid, p->name, p->burst_time, p->arrival_time,
                   p->completion_time, p->waiting_time, p->turnaround_time);
            count++;
        }
    }

    if (count > 0) {
        std::printf("\n  Average Waiting Time:    %.2f\n", avg_waiting_time);
        std::printf("  Average Turnaround Time: %.2f\n", avg_turnaround_time);
    }
}

int sched_list_processes(void) {
    std::printf("  %-6s %-16s %-12s %-8s %-8s %-10s %-8s\n",
           "PID", "Name", "State", "Burst", "Arrival", "Remaining", "Role");
    std::printf("  %-6s %-16s %-12s %-8s %-8s %-10s %-8s\n",
           "---", "----", "-----", "-----", "-------", "---------", "----");

    int found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].active) {
            PCB *p = &process_table[i];
            std::printf("  %-6d %-16s %-12s %-8d %-8d %-10d %-8s\n",
                   p->pid, p->name, proc_state_str(p->state),
                   p->burst_time, p->arrival_time, p->remaining_time,
                   role_str(p->role));
            found++;
        }
    }

    if (found == 0) {
        std::printf("  (no active processes)\n");
    }

    return MOSS_SUCCESS;
}

void sched_cleanup(void) {
    std::memset(process_table, 0, sizeof(process_table));
    process_count = 0;
    next_pid = 0;
    running_pid = -1;
    gantt_count = 0;
    moss_log(LOG_INFO, "Scheduler subsystem cleaned up");
}

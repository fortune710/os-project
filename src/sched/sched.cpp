#include "sched.h"

moss_status_t sched_init(sched_policy_t policy, moss_time_t quantum) {
    (void)policy;
    (void)quantum;
    return MOSS_ENOTREADY;
}

moss_status_t sched_create_process(const sched_process_t *proc) {
    (void)proc;
    return MOSS_ENOTREADY;
}

moss_status_t sched_terminate_process(moss_pid_t pid) {
    (void)pid;
    return MOSS_ENOTREADY;
}

moss_status_t sched_tick(moss_time_t delta) {
    (void)delta;
    return MOSS_ENOTREADY;
}

moss_status_t sched_schedule(moss_pid_t *scheduled_pid) {
    (void)scheduled_pid;
    return MOSS_ENOTREADY;
}

moss_status_t sched_get_stats(sched_stats_t *out_stats) {
    (void)out_stats;
    return MOSS_ENOTREADY;
}

moss_status_t sched_set_policy(sched_policy_t policy, moss_time_t quantum) {
    (void)policy;
    (void)quantum;
    return MOSS_ENOTREADY;
}

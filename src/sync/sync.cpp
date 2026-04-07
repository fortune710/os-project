#include "sync.h"

moss_status_t sync_init(void) {
    return MOSS_ENOTREADY;
}

moss_status_t sync_mutex_create(const char *name) {
    (void)name;
    return MOSS_ENOTREADY;
}

moss_status_t sync_mutex_lock(const char *name) {
    (void)name;
    return MOSS_ENOTREADY;
}

moss_status_t sync_mutex_unlock(const char *name) {
    (void)name;
    return MOSS_ENOTREADY;
}

moss_status_t sync_sem_create(const char *name, int initial_count) {
    (void)name;
    (void)initial_count;
    return MOSS_ENOTREADY;
}

moss_status_t sync_sem_wait(const char *name) {
    (void)name;
    return MOSS_ENOTREADY;
}

moss_status_t sync_sem_post(const char *name) {
    (void)name;
    return MOSS_ENOTREADY;
}

moss_status_t sync_check_access(const sync_access_t *access) {
    (void)access;
    return MOSS_ENOTREADY;
}

moss_status_t sync_run_scenario(const char *scenario_name) {
    (void)scenario_name;
    return MOSS_ENOTREADY;
}

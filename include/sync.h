#ifndef MOSS_SYNC_H
#define MOSS_SYNC_H

#include "moss.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYNC_ROLE_USER = 0,
    SYNC_ROLE_ADMIN = 1
} sync_role_t;

typedef struct {
    const char *resource_id;
    sync_role_t role;
} sync_access_t;

moss_status_t sync_init(void);

moss_status_t sync_mutex_create(const char *name);
moss_status_t sync_mutex_lock(const char *name);
moss_status_t sync_mutex_unlock(const char *name);

moss_status_t sync_sem_create(const char *name, int initial_count);
moss_status_t sync_sem_wait(const char *name);
moss_status_t sync_sem_post(const char *name);

moss_status_t sync_check_access(const sync_access_t *access);
moss_status_t sync_run_scenario(const char *scenario_name);

#ifdef __cplusplus
}
#endif

#endif

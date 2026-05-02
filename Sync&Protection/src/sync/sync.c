#include "sync.h"
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#ifdef __APPLE__
#include <fcntl.h>
#endif

#define MAX_MUTEXES 10
#define MAX_SEMAPHORES 10

// Internal subsystem state
static pthread_mutex_t mutex_pool[MAX_MUTEXES];
#ifdef __APPLE__
#define SEM_NAME_LEN 64
static sem_t* sem_pool[MAX_SEMAPHORES];
static char sem_names[MAX_SEMAPHORES][SEM_NAME_LEN];
#else
static sem_t sem_pool[MAX_SEMAPHORES];
#endif

// Role management state
#define MAX_PROCESSES 100
static int process_roles[MAX_PROCESSES]; // 0: User, 1: Admin

// --- Mutex Implementation ---
int sync_mutex_init(int mutex_id) {
    if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) return -1;
    return pthread_mutex_init(&mutex_pool[mutex_id], NULL) == 0 ? 0 : -2;
}

int sync_mutex_lock(int mutex_id) {
    if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) return -1;
    return pthread_mutex_lock(&mutex_pool[mutex_id]) == 0 ? 0 : -2;
}

int sync_mutex_unlock(int mutex_id) {
    if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) return -1;
    return pthread_mutex_unlock(&mutex_pool[mutex_id]) == 0 ? 0 : -2;
}

// --- Semaphore Implementation ---
int sync_sem_init(int sem_id, int initial_value) {
    if (sem_id < 0 || sem_id >= MAX_SEMAPHORES) return -1;
#ifdef __APPLE__
    if (sem_pool[sem_id] != NULL) {
        sem_close(sem_pool[sem_id]);
        (void)sem_unlink(sem_names[sem_id]);
        sem_pool[sem_id] = NULL;
    }

    snprintf(sem_names[sem_id], sizeof(sem_names[sem_id]), "/moss_sem_%d_%d", (int)getpid(), sem_id);
    sem_pool[sem_id] = sem_open(sem_names[sem_id], O_CREAT | O_EXCL, 0600, (unsigned int)initial_value);
    if (sem_pool[sem_id] == SEM_FAILED) {
        sem_pool[sem_id] = NULL;
        (void)sem_unlink(sem_names[sem_id]);
        sem_pool[sem_id] = sem_open(sem_names[sem_id], O_CREAT, 0600, (unsigned int)initial_value);
    }
    return (sem_pool[sem_id] == SEM_FAILED || sem_pool[sem_id] == NULL) ? -2 : 0;
#else
    return sem_init(&sem_pool[sem_id], 0, (unsigned int)initial_value) == 0 ? 0 : -2;
#endif
}

int sync_sem_wait(int sem_id) {
    if (sem_id < 0 || sem_id >= MAX_SEMAPHORES) return -1;
#ifdef __APPLE__
    if (sem_pool[sem_id] == NULL) return -2;
    return sem_wait(sem_pool[sem_id]) == 0 ? 0 : -2;
#else
    return sem_wait(&sem_pool[sem_id]) == 0 ? 0 : -2;
#endif
}

int sync_sem_post(int sem_id) {
    if (sem_id < 0 || sem_id >= MAX_SEMAPHORES) return -1;
#ifdef __APPLE__
    if (sem_pool[sem_id] == NULL) return -2;
    return sem_post(sem_pool[sem_id]) == 0 ? 0 : -2;
#else
    return sem_post(&sem_pool[sem_id]) == 0 ? 0 : -2;
#endif
}

// --- Protection Implementation ---
int sync_set_process_role(int process_id, int role_id) {
    if (process_id < 0 || process_id >= MAX_PROCESSES) return -1;
    process_roles[process_id] = role_id;
    return 0;
}

int sync_check_permission(int process_id, int resource_id) {
    if (process_id < 0 || process_id >= MAX_PROCESSES) return -1;

    // Basic conceptual protection: Even resources require Admin (1), Odd resources allow User (0)
    int required_role = (resource_id % 2 == 0) ? 1 : 0;

    if (process_roles[process_id] >= required_role) {
        return 0; // Access granted
    }
    return -3; // Permission denied
}

// --- Producer-Consumer Simulation ---
#define BUFFER_SIZE 5
static int buffer[BUFFER_SIZE];
static int in_index = 0;
static int out_index = 0;
static int pc_initialized = 0;

// Hardcoded IDs for the simulation using the internal pools
#define PC_MUTEX_ID 0
#define PC_SEM_EMPTY_ID 0
#define PC_SEM_FULL_ID 1

int sync_pc_init(void) {
    int status;

    if (!pc_initialized) {
        status = sync_mutex_init(PC_MUTEX_ID);
        if (status != 0) return -4;

        status = sync_sem_init(PC_SEM_EMPTY_ID, BUFFER_SIZE);
        if (status != 0) return -4;

        status = sync_sem_init(PC_SEM_FULL_ID, 0);
        if (status != 0) return -4;

        pc_initialized = 1;
        return 0;
    }

    return sync_pc_reset();
}

int sync_pc_reset(void) {
    if (!pc_initialized) return -4;

    if (sync_mutex_lock(PC_MUTEX_ID) != 0) return -5;
    in_index = 0;
    out_index = 0;
    memset(buffer, 0, sizeof(buffer));
    if (sync_mutex_unlock(PC_MUTEX_ID) != 0) return -5;

#ifdef __APPLE__
    if (sem_pool[PC_SEM_EMPTY_ID] != NULL) {
        if (sem_close(sem_pool[PC_SEM_EMPTY_ID]) != 0) return -4;
        (void)sem_unlink(sem_names[PC_SEM_EMPTY_ID]);
        sem_pool[PC_SEM_EMPTY_ID] = NULL;
    }
    if (sem_pool[PC_SEM_FULL_ID] != NULL) {
        if (sem_close(sem_pool[PC_SEM_FULL_ID]) != 0) return -4;
        (void)sem_unlink(sem_names[PC_SEM_FULL_ID]);
        sem_pool[PC_SEM_FULL_ID] = NULL;
    }
#else
    if (sem_destroy(&sem_pool[PC_SEM_EMPTY_ID]) != 0) return -4;
    if (sem_destroy(&sem_pool[PC_SEM_FULL_ID]) != 0) return -4;
#endif
    if (sync_sem_init(PC_SEM_EMPTY_ID, BUFFER_SIZE) != 0) return -4;
    if (sync_sem_init(PC_SEM_FULL_ID, 0) != 0) return -4;

    return 0;
}

int sync_produce(int item) {
    if (sync_sem_wait(PC_SEM_EMPTY_ID) != 0) return -5;
    if (sync_mutex_lock(PC_MUTEX_ID) != 0) return -5;

    buffer[in_index] = item;
    in_index = (in_index + 1) % BUFFER_SIZE;

    sync_mutex_unlock(PC_MUTEX_ID);
    sync_sem_post(PC_SEM_FULL_ID);

    return 0;
}

int sync_consume(int *item) {
    if (sync_sem_wait(PC_SEM_FULL_ID) != 0) return -5;
    if (sync_mutex_lock(PC_MUTEX_ID) != 0) return -5;

    *item = buffer[out_index];
    out_index = (out_index + 1) % BUFFER_SIZE;

    sync_mutex_unlock(PC_MUTEX_ID);
    sync_sem_post(PC_SEM_EMPTY_ID);

    return 0;
}

#ifndef SYNC_H
#define SYNC_H

int sync_mutex_init(int mutex_id);
int sync_mutex_lock(int mutex_id);
int sync_mutex_unlock(int mutex_id);

int sync_sem_init(int sem_id, int initial_value);
int sync_sem_wait(int sem_id);
int sync_sem_post(int sem_id);

int sync_set_process_role(int process_id, int role_id);
int sync_check_permission(int process_id, int resource_id);

// Producer-Consumer API
int sync_pc_init(void);
int sync_pc_reset(void);
int sync_produce(int item);
int sync_consume(int *item);

#endif

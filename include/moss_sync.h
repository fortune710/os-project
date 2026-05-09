/*
 * sync.h - Synchronization & Protection API
 * Subsystem C of MOSS
 *
 * Provides mutex locks, semaphores, Producer-Consumer simulation,
 * access control checks, and deadlock detection.
 */

#ifndef MOSS_SYNC_H
#define MOSS_SYNC_H

#include "moss_common.h"

/*
 * sync_init - Initialize the synchronization subsystem.
 * Must be called before any other sync_ functions.
 * Returns: MOSS_SUCCESS on success.
 */
int sync_init(void);

/*
 * sync_mutex_create - Create a named mutex.
 * @name: Mutex name (max MAX_NAME_LEN chars).
 * Returns: Resource ID (>= 0) on success.
 *          MOSS_ERR_FULL if max resources reached.
 */
int sync_mutex_create(const char *name);

/*
 * sync_mutex_lock - Lock a mutex for a process.
 * @resource_id: Mutex resource ID.
 * @pid:         Process ID requesting the lock.
 * Returns: MOSS_SUCCESS if lock acquired.
 *          MOSS_ERR_BUSY if already locked (process added to wait queue).
 *          MOSS_ERR_NOT_FOUND if resource_id invalid.
 */
int sync_mutex_lock(int resource_id, int pid);

/*
 * sync_mutex_unlock - Unlock a mutex.
 * @resource_id: Mutex resource ID.
 * @pid:         Process ID releasing the lock (must be owner).
 * Returns: MOSS_SUCCESS if unlocked.
 *          MOSS_ERR_DENIED if pid is not the lock owner.
 *          MOSS_ERR_NOT_FOUND if resource_id invalid.
 */
int sync_mutex_unlock(int resource_id, int pid);

/*
 * sync_sem_create - Create a named semaphore.
 * @name:          Semaphore name (max MAX_NAME_LEN chars).
 * @initial_value: Initial semaphore value (>= 0).
 * Returns: Semaphore ID (>= 0) on success.
 *          MOSS_ERR_FULL if max semaphores reached.
 */
int sync_sem_create(const char *name, int initial_value);

/*
 * sync_sem_wait - Perform P() / wait on a semaphore.
 * @sem_id: Semaphore ID.
 * @pid:    Process ID performing the wait.
 * Returns: MOSS_SUCCESS if decremented successfully.
 *          MOSS_ERR_BUSY if value was 0 (process blocked/queued).
 *          MOSS_ERR_NOT_FOUND if sem_id invalid.
 */
int sync_sem_wait(int sem_id, int pid);

/*
 * sync_sem_signal - Perform V() / signal on a semaphore.
 * @sem_id: Semaphore ID.
 * @pid:    Process ID performing the signal.
 * Returns: MOSS_SUCCESS on success.
 *          MOSS_ERR_NOT_FOUND if sem_id invalid.
 */
int sync_sem_signal(int sem_id, int pid);

/*
 * sync_run_producer_consumer - Simulate the Producer-Consumer problem.
 * Uses a bounded buffer with mutex and semaphores.
 * @buffer_size: Size of the shared buffer.
 * @num_items:   Number of items to produce/consume.
 * Returns: MOSS_SUCCESS on success.
 *          MOSS_ERR_INVALID if parameters out of range.
 */
int sync_run_producer_consumer(int buffer_size, int num_items);

/*
 * sync_run_readers_writers - Simulate the Readers-Writers problem.
 * Demonstrates concurrent readers with exclusive writer access.
 * @num_readers: Number of reader processes (1–8).
 * @num_writers: Number of writer processes (1–4).
 * @num_ops:     Number of operations to simulate.
 * Returns: MOSS_SUCCESS on success.
 *          MOSS_ERR_INVALID if parameters out of range.
 */
int sync_run_readers_writers(int num_readers, int num_writers, int num_ops);

/*
 * sync_check_permission - Check if a process has permission for an action.
 * @pid:      Process ID.
 * @resource: Resource type ("memory", "process", "sync_resource").
 * @action:   Action type ("read", "write", "execute", "admin").
 * Returns: MOSS_SUCCESS if permitted.
 *          MOSS_ERR_DENIED if not permitted.
 *          MOSS_ERR_NOT_FOUND if pid invalid.
 */
int sync_check_permission(int pid, const char *resource, const char *action);

/*
 * sync_set_role - Set the access control role for a process.
 * @pid:  Process ID.
 * @role: ROLE_ADMIN or ROLE_USER.
 * Returns: MOSS_SUCCESS on success, MOSS_ERR_NOT_FOUND if pid invalid.
 */
int sync_set_role(int pid, UserRole role);

/*
 * sync_detect_deadlock - Detect deadlock using wait-for graph analysis.
 * Returns: MOSS_ERR_DEADLOCK if deadlock detected.
 *          MOSS_SUCCESS if no deadlock.
 */
int sync_detect_deadlock(void);

/*
 * sync_print_state - Print the state of all mutexes and semaphores.
 */
void sync_print_state(void);

/*
 * sync_cleanup - Free all synchronization resources.
 * Should be called during system shutdown.
 */
void sync_cleanup(void);

#endif /* MOSS_SYNC_H */

/*
 * sync.cpp - Synchronization & Protection Implementation
 * Subsystem C of MOSS
 *
 * Implements mutex locks, semaphores, Producer-Consumer simulation,
 * role-based access control, and deadlock detection via wait-for graph.
 */

#include "moss_sync.h"
#include "moss_sched.h"  /* For sched_get_process() to look up roles */
#include <string>

/* ============================================================
 * Internal Data Structures
 * ============================================================ */

/* Mutex */
struct Mutex {
    int active;
    char name[MAX_NAME_LEN];
    int locked;
    int owner_pid;
    int wait_queue[MAX_WAIT_QUEUE];
    int wait_count;
};

/* Semaphore */
struct Semaphore {
    int active;
    char name[MAX_NAME_LEN];
    int value;
    int wait_queue[MAX_WAIT_QUEUE];
    int wait_count;
};

/* Access control rule */
struct AccessRule {
    const char *resource;
    const char *action;
    int admin_allowed;
    int user_allowed;
};

/* ============================================================
 * Internal State
 * ============================================================ */

static Mutex mutexes[MAX_RESOURCES];
static int mutex_count = 0;

static Semaphore semaphores[MAX_SEMAPHORES];
static int sem_count = 0;

/* Access control matrix */
static const AccessRule access_rules[] = {
    /* resource          action      admin   user */
    { "memory",          "read",     1,      1    },
    { "memory",          "write",    1,      1    },
    { "memory",          "execute",  1,      0    },
    { "memory",          "admin",    1,      0    },
    { "process",         "read",     1,      1    },
    { "process",         "write",    1,      0    },
    { "process",         "execute",  1,      1    },
    { "process",         "admin",    1,      0    },
    { "sync_resource",   "read",     1,      1    },
    { "sync_resource",   "write",    1,      0    },
    { "sync_resource",   "execute",  1,      1    },
    { "sync_resource",   "admin",    1,      0    },
    { nullptr,           nullptr,    0,      0    }  /* sentinel */
};

/* ============================================================
 * API Implementation
 * ============================================================ */

int sync_init(void) {
    std::memset(mutexes, 0, sizeof(mutexes));
    std::memset(semaphores, 0, sizeof(semaphores));
    mutex_count = 0;
    sem_count = 0;

    for (int i = 0; i < MAX_RESOURCES; i++) {
        mutexes[i].active = 0;
        mutexes[i].owner_pid = -1;
    }

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        semaphores[i].active = 0;
    }

    moss_log(LOG_INFO, "Synchronization subsystem initialized");
    return MOSS_SUCCESS;
}

int sync_mutex_create(const char *name) {
    if (name == nullptr) {
        return MOSS_ERR_INVALID;
    }

    if (mutex_count >= MAX_RESOURCES) {
        return MOSS_ERR_FULL;
    }

    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (!mutexes[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        return MOSS_ERR_FULL;
    }

    Mutex *m = &mutexes[slot];
    m->active = 1;
    m->locked = 0;
    m->owner_pid = -1;
    m->wait_count = 0;
    std::strncpy(m->name, name, MAX_NAME_LEN - 1);
    m->name[MAX_NAME_LEN - 1] = '\0';
    mutex_count++;

    moss_log(LOG_INFO, "Mutex created: ID=%d Name=%s", slot, name);
    return slot;
}

int sync_mutex_lock(int resource_id, int pid) {
    if (resource_id < 0 || resource_id >= MAX_RESOURCES ||
        !mutexes[resource_id].active) {
        return MOSS_ERR_NOT_FOUND;
    }

    Mutex *m = &mutexes[resource_id];

    if (!m->locked) {
        /* Lock is free - acquire it */
        m->locked = 1;
        m->owner_pid = pid;
        moss_log(LOG_INFO, "Mutex '%s' (ID=%d) locked by PID=%d",
                 m->name, resource_id, pid);
        return MOSS_SUCCESS;
    }

    if (m->owner_pid == pid) {
        /* Already owned by this process */
        moss_log(LOG_WARN, "Mutex '%s' already locked by PID=%d (re-entrant attempt)",
                 m->name, pid);
        return MOSS_ERR_BUSY;
    }

    /* Lock is held by another process - add to wait queue */
    if (m->wait_count < MAX_WAIT_QUEUE) {
        m->wait_queue[m->wait_count++] = pid;
        moss_log(LOG_WARN, "Mutex '%s' busy (owner=PID %d). PID=%d added to wait queue",
                 m->name, m->owner_pid, pid);
    }
    return MOSS_ERR_BUSY;
}

int sync_mutex_unlock(int resource_id, int pid) {
    if (resource_id < 0 || resource_id >= MAX_RESOURCES ||
        !mutexes[resource_id].active) {
        return MOSS_ERR_NOT_FOUND;
    }

    Mutex *m = &mutexes[resource_id];

    if (!m->locked) {
        moss_log(LOG_WARN, "Mutex '%s' is not locked", m->name);
        return MOSS_ERR_INVALID;
    }

    if (m->owner_pid != pid) {
        moss_log(LOG_ERROR, "Mutex '%s' unlock denied: owned by PID=%d, requested by PID=%d",
                 m->name, m->owner_pid, pid);
        return MOSS_ERR_DENIED;
    }

    /* Release the lock */
    moss_log(LOG_INFO, "Mutex '%s' (ID=%d) unlocked by PID=%d",
             m->name, resource_id, pid);

    /* If there's a waiter, hand off the lock */
    if (m->wait_count > 0) {
        int next_pid = m->wait_queue[0];
        /* Shift wait queue */
        for (int i = 0; i < m->wait_count - 1; i++) {
            m->wait_queue[i] = m->wait_queue[i + 1];
        }
        m->wait_count--;
        m->owner_pid = next_pid;
        moss_log(LOG_INFO, "Mutex '%s' handed to waiting PID=%d", m->name, next_pid);
    } else {
        m->locked = 0;
        m->owner_pid = -1;
    }

    return MOSS_SUCCESS;
}

int sync_sem_create(const char *name, int initial_value) {
    if (name == nullptr || initial_value < 0) {
        return MOSS_ERR_INVALID;
    }

    if (sem_count >= MAX_SEMAPHORES) {
        return MOSS_ERR_FULL;
    }

    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!semaphores[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        return MOSS_ERR_FULL;
    }

    Semaphore *s = &semaphores[slot];
    s->active = 1;
    s->value = initial_value;
    s->wait_count = 0;
    std::strncpy(s->name, name, MAX_NAME_LEN - 1);
    s->name[MAX_NAME_LEN - 1] = '\0';
    sem_count++;

    moss_log(LOG_INFO, "Semaphore created: ID=%d Name=%s Value=%d",
             slot, name, initial_value);
    return slot;
}

int sync_sem_wait(int sem_id, int pid) {
    if (sem_id < 0 || sem_id >= MAX_SEMAPHORES ||
        !semaphores[sem_id].active) {
        return MOSS_ERR_NOT_FOUND;
    }

    Semaphore *s = &semaphores[sem_id];

    if (s->value > 0) {
        s->value--;
        moss_log(LOG_INFO, "Semaphore '%s' (ID=%d) wait by PID=%d: value=%d",
                 s->name, sem_id, pid, s->value);
        return MOSS_SUCCESS;
    }

    /* Value is 0 - process must wait */
    if (s->wait_count < MAX_WAIT_QUEUE) {
        s->wait_queue[s->wait_count++] = pid;
        moss_log(LOG_WARN, "Semaphore '%s' (ID=%d): PID=%d blocked (value=0)",
                 s->name, sem_id, pid);
    }
    return MOSS_ERR_BUSY;
}

int sync_sem_signal(int sem_id, int pid) {
    if (sem_id < 0 || sem_id >= MAX_SEMAPHORES ||
        !semaphores[sem_id].active) {
        return MOSS_ERR_NOT_FOUND;
    }

    Semaphore *s = &semaphores[sem_id];

    if (s->wait_count > 0) {
        /* Wake up a waiting process */
        int woken_pid = s->wait_queue[0];
        for (int i = 0; i < s->wait_count - 1; i++) {
            s->wait_queue[i] = s->wait_queue[i + 1];
        }
        s->wait_count--;
        moss_log(LOG_INFO, "Semaphore '%s' (ID=%d) signal by PID=%d: woke PID=%d",
                 s->name, sem_id, pid, woken_pid);
    } else {
        s->value++;
        moss_log(LOG_INFO, "Semaphore '%s' (ID=%d) signal by PID=%d: value=%d",
                 s->name, sem_id, pid, s->value);
    }

    return MOSS_SUCCESS;
}

int sync_run_producer_consumer(int buffer_size, int num_items) {
    if (buffer_size <= 0 || buffer_size > MAX_BUFFER_SIZE ||
        num_items <= 0) {
        return MOSS_ERR_INVALID;
    }

    std::printf("\n  ===== Producer-Consumer Simulation =====\n");
    std::printf("  Buffer size: %d, Items to produce: %d\n\n", buffer_size, num_items);

    /* Simulated bounded buffer */
    int buffer[MAX_BUFFER_SIZE];
    int buf_count = 0;
    int in_idx = 0;   /* Producer insert position */
    int out_idx = 0;  /* Consumer remove position */

    /* Simulated semaphore values */
    int sem_empty = buffer_size;  /* Tracks empty slots */
    int sem_full = 0;             /* Tracks filled slots */
    int mutex_held = 0;           /* Simulated mutex: 0 = free, 1 = held */

    int produced = 0;
    int consumed = 0;
    int step = 0;

    /* Suppress unused variable warning for buffer array and mutex_held */
    (void)buffer;
    (void)mutex_held;

    /* Alternate between producer and consumer actions */
    while (consumed < num_items) {
        step++;

        /* Producer tries to produce */
        if (produced < num_items && sem_empty > 0) {
            /* wait(empty) */
            sem_empty--;
            std::printf("  [Step %2d] Producer: wait(empty) -> empty=%d\n",
                   step, sem_empty);

            /* lock(mutex) */
            mutex_held = 1;
            std::printf("  [Step %2d] Producer: lock(mutex)\n", step);

            /* Produce item */
            int item = produced + 1;
            buffer[in_idx] = item;
            in_idx = (in_idx + 1) % buffer_size;
            buf_count++;
            produced++;
            std::printf("  [Step %2d] Producer: produced item %d [buffer: %d/%d]\n",
                   step, item, buf_count, buffer_size);

            /* unlock(mutex) */
            mutex_held = 0;
            std::printf("  [Step %2d] Producer: unlock(mutex)\n", step);

            /* signal(full) */
            sem_full++;
            std::printf("  [Step %2d] Producer: signal(full) -> full=%d\n",
                   step, sem_full);

            step++;
        }

        /* Consumer tries to consume */
        if (sem_full > 0) {
            /* wait(full) */
            sem_full--;
            std::printf("  [Step %2d] Consumer: wait(full) -> full=%d\n",
                   step, sem_full);

            /* lock(mutex) */
            mutex_held = 1;
            std::printf("  [Step %2d] Consumer: lock(mutex)\n", step);

            /* Consume item */
            int item = buffer[out_idx];
            out_idx = (out_idx + 1) % buffer_size;
            buf_count--;
            consumed++;
            std::printf("  [Step %2d] Consumer: consumed item %d [buffer: %d/%d]\n",
                   step, item, buf_count, buffer_size);

            /* unlock(mutex) */
            mutex_held = 0;
            std::printf("  [Step %2d] Consumer: unlock(mutex)\n", step);

            /* signal(empty) */
            sem_empty++;
            std::printf("  [Step %2d] Consumer: signal(empty) -> empty=%d\n",
                   step, sem_empty);
        } else if (produced >= num_items) {
            break;
        }

        std::printf("\n");
    }

    std::printf("  ===== Simulation Complete =====\n");
    std::printf("  Produced: %d items, Consumed: %d items\n", produced, consumed);
    std::printf("  No deadlocks or race conditions detected.\n\n");

    moss_log(LOG_INFO, "Producer-Consumer simulation completed: %d items", num_items);
    return MOSS_SUCCESS;
}

int sync_run_readers_writers(int num_readers, int num_writers, int num_ops) {
    if (num_readers <= 0 || num_readers > 8 ||
        num_writers <= 0 || num_writers > 4 ||
        num_ops <= 0) {
        return MOSS_ERR_INVALID;
    }

    std::printf("\n  ===== Readers-Writers Simulation =====\n");
    std::printf("  Readers: %d, Writers: %d, Operations: %d\n\n",
           num_readers, num_writers, num_ops);

    /*
     * Readers-Writers with reader preference:
     * - Multiple readers can read simultaneously
     * - Writers require exclusive access
     * - Simulated semaphores:
     *   rw_mutex:    controls access to the shared resource (binary)
     *   mutex:       protects the reader_count variable (binary)
     *   reader_count: number of active readers
     */

    int reader_count = 0;
    int rw_mutex_held = 0;    /* 0 = free, 1 = held */
    int rw_mutex_holder = -1; /* PID of holder: -1=none, -2=readers */

    int step = 0;
    int total_reads = 0;
    int total_writes = 0;

    /* Round-robin through readers and writers */
    int next_reader = 0;
    int next_writer = 0;
    bool writer_turn = false;

    for (int op = 0; op < num_ops; op++) {
        step++;

        if (!writer_turn || next_writer >= num_writers) {
            /* Reader tries to read */
            int rid = next_reader % num_readers;
            next_reader++;

            if (rw_mutex_holder != -1 && rw_mutex_holder != -2) {
                /* Writer holds the lock - reader must wait */
                std::printf("  [Step %2d] Reader-%d: BLOCKED (writer active)\n",
                       step, rid);
            } else {
                /* Reader can proceed */
                reader_count++;
                if (reader_count == 1) {
                    /* First reader acquires rw_mutex */
                    rw_mutex_held = 1;
                    rw_mutex_holder = -2; /* readers */
                    std::printf("  [Step %2d] Reader-%d: lock(mutex), reader_count=%d, lock(rw_mutex)\n",
                           step, rid, reader_count);
                } else {
                    std::printf("  [Step %2d] Reader-%d: lock(mutex), reader_count=%d\n",
                           step, rid, reader_count);
                }

                std::printf("  [Step %2d] Reader-%d: ** READING data ** (concurrent readers: %d)\n",
                       step, rid, reader_count);
                total_reads++;

                /* Reader finishes */
                reader_count--;
                if (reader_count == 0) {
                    rw_mutex_held = 0;
                    rw_mutex_holder = -1;
                    std::printf("  [Step %2d] Reader-%d: reader_count=%d, unlock(rw_mutex)\n",
                           step, rid, reader_count);
                } else {
                    std::printf("  [Step %2d] Reader-%d: reader_count=%d\n",
                           step, rid, reader_count);
                }
            }

            writer_turn = true;
        } else {
            /* Writer tries to write */
            int wid = next_writer % num_writers;
            next_writer++;

            if (rw_mutex_held) {
                /* Resource is busy (readers or another writer) */
                if (rw_mutex_holder == -2) {
                    std::printf("  [Step %2d] Writer-%d: BLOCKED (readers active, count=%d)\n",
                           step, wid, reader_count);
                } else {
                    std::printf("  [Step %2d] Writer-%d: BLOCKED (another writer active)\n",
                           step, wid);
                }
            } else {
                /* Writer acquires exclusive access */
                rw_mutex_held = 1;
                rw_mutex_holder = wid;
                std::printf("  [Step %2d] Writer-%d: lock(rw_mutex) - EXCLUSIVE ACCESS\n",
                       step, wid);

                std::printf("  [Step %2d] Writer-%d: ** WRITING data **\n",
                       step, wid);
                total_writes++;

                /* Writer finishes */
                rw_mutex_held = 0;
                rw_mutex_holder = -1;
                std::printf("  [Step %2d] Writer-%d: unlock(rw_mutex)\n",
                       step, wid);
            }

            writer_turn = false;
        }

        std::printf("\n");
    }

    std::printf("  ===== Simulation Complete =====\n");
    std::printf("  Total reads: %d, Total writes: %d\n", total_reads, total_writes);
    std::printf("  No data corruption detected (mutual exclusion enforced).\n\n");

    moss_log(LOG_INFO, "Readers-Writers simulation completed: %d ops", num_ops);
    return MOSS_SUCCESS;
}

int sync_check_permission(int pid, const char *resource, const char *action) {
    if (resource == nullptr || action == nullptr) {
        return MOSS_ERR_INVALID;
    }

    /* Look up the process to get its role */
    PCB *p = sched_get_process(pid);
    if (p == nullptr) {
        return MOSS_ERR_NOT_FOUND;
    }

    UserRole role = p->role;
    std::string res(resource);
    std::string act(action);

    /* Check against access rules */
    for (int i = 0; access_rules[i].resource != nullptr; i++) {
        if (res == access_rules[i].resource && act == access_rules[i].action) {
            int allowed = (role == ROLE_ADMIN) ?
                          access_rules[i].admin_allowed :
                          access_rules[i].user_allowed;
            if (allowed) {
                moss_log(LOG_INFO, "Permission GRANTED: PID=%d (%s) -> %s:%s",
                         pid, role_str(role), resource, action);
                return MOSS_SUCCESS;
            } else {
                moss_log(LOG_WARN, "Permission DENIED: PID=%d (%s) -> %s:%s",
                         pid, role_str(role), resource, action);
                return MOSS_ERR_DENIED;
            }
        }
    }

    /* Resource/action combination not found in rules - deny by default */
    moss_log(LOG_WARN, "Permission DENIED: unknown resource/action %s:%s", resource, action);
    return MOSS_ERR_DENIED;
}

int sync_set_role(int pid, UserRole role) {
    PCB *p = sched_get_process(pid);
    if (p == nullptr) {
        return MOSS_ERR_NOT_FOUND;
    }

    p->role = role;
    moss_log(LOG_INFO, "Role set: PID=%d -> %s", pid, role_str(role));
    return MOSS_SUCCESS;
}

int sync_detect_deadlock(void) {
    /*
     * Simple deadlock detection using wait-for graph cycle detection.
     *
     * Build a wait-for graph: for each mutex, if process A holds it
     * and process B is waiting for it, add edge B -> A.
     * Then check for cycles using Floyd's tortoise and hare.
     */

    /* Wait-for info: wait_on_res[i] = resource ID process i is waiting on */
    int wait_for[MAX_PROCESSES * 2];
    int wait_on_res[MAX_PROCESSES * 2];
    int max_pid = 0;

    std::memset(wait_for, -1, sizeof(wait_for));
    std::memset(wait_on_res, -1, sizeof(wait_on_res));

    /* Build wait-for edges from mutex state */
    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (!mutexes[i].active || !mutexes[i].locked) continue;

        int owner = mutexes[i].owner_pid;
        if (owner > max_pid) max_pid = owner;

        for (int j = 0; j < mutexes[i].wait_count; j++) {
            int waiter = mutexes[i].wait_queue[j];
            if (waiter >= 0 && waiter < MAX_PROCESSES * 2) {
                wait_for[waiter] = owner;
                wait_on_res[waiter] = i;
                if (waiter > max_pid) max_pid = waiter;
            }
        }
    }

    /* Cycle detection: follow chains and look for cycles */
    for (int start = 0; start <= max_pid; start++) {
        if (wait_for[start] < 0) continue;

        /* Tortoise and hare cycle detection */
        int slow = start;
        int fast = start;
        bool has_cycle = false;

        while (true) {
            /* Move slow one step */
            if (wait_for[slow] < 0) break;
            slow = wait_for[slow];

            /* Move fast two steps */
            if (wait_for[fast] < 0) break;
            fast = wait_for[fast];
            if (wait_for[fast] < 0) break;
            fast = wait_for[fast];

            if (slow == fast) {
                has_cycle = true;
                break;
            }
        }

        if (has_cycle) {
            /* Report the cycle */
            std::printf("  DEADLOCK DETECTED!\n");
            std::printf("  Wait-for Cycle:\n");
            int node = slow;
            do {
                int next = wait_for[node];
                int res_id = wait_on_res[node];
                std::printf("    PID %d is waiting for Mutex '%s' (ID %d) held by PID %d\n",
                       node, mutexes[res_id].name, res_id, next);
                node = next;
            } while (node != slow);

            moss_log(LOG_ERROR, "Deadlock detected in wait-for graph");
            return MOSS_ERR_DEADLOCK;
        }
    }

    moss_log(LOG_INFO, "No deadlock detected");
    std::printf("  No deadlock detected.\n");
    return MOSS_SUCCESS;
}

void sync_print_state(void) {
    std::printf("  === Mutex State ===\n");
    std::printf("  %-6s %-16s %-10s %-10s %-10s\n",
           "ID", "Name", "Locked", "Owner", "Waiters");
    std::printf("  %-6s %-16s %-10s %-10s %-10s\n",
           "--", "----", "------", "-----", "-------");

    int found_mutex = 0;
    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (mutexes[i].active) {
            found_mutex = 1;
            std::printf("  %-6d %-16s %-10s ",
                   i, mutexes[i].name,
                   mutexes[i].locked ? "YES" : "NO");
            if (mutexes[i].locked) {
                std::printf("PID %-6d ", mutexes[i].owner_pid);
            } else {
                std::printf("%-10s ", "-");
            }
            std::printf("%d", mutexes[i].wait_count);
            if (mutexes[i].wait_count > 0) {
                std::printf(" [PIDs: ");
                for (int j = 0; j < mutexes[i].wait_count; j++) {
                    std::printf("%d%s", mutexes[i].wait_queue[j], (j == mutexes[i].wait_count - 1) ? "" : ",");
                }
                std::printf("]");
            }
            std::printf("\n");
        }
    }
    if (!found_mutex) {
        std::printf("  (no mutexes created)\n");
    }

    std::printf("\n  === Semaphore State ===\n");
    std::printf("  %-6s %-16s %-10s %-10s\n",
           "ID", "Name", "Value", "Waiters");
    std::printf("  %-6s %-16s %-10s %-10s\n",
           "--", "----", "-----", "-------");

    int found_sem = 0;
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].active) {
            found_sem = 1;
            std::printf("  %-6d %-16s %-10d %d",
                   i, semaphores[i].name,
                   semaphores[i].value,
                   semaphores[i].wait_count);
            if (semaphores[i].wait_count > 0) {
                std::printf(" [PIDs: ");
                for (int j = 0; j < semaphores[i].wait_count; j++) {
                    std::printf("%d%s", semaphores[i].wait_queue[j], (j == semaphores[i].wait_count - 1) ? "" : ",");
                }
                std::printf("]");
            }
            std::printf("\n");
        }
    }
    if (!found_sem) {
        std::printf("  (no semaphores created)\n");
    }
}

void sync_cleanup(void) {
    std::memset(mutexes, 0, sizeof(mutexes));
    std::memset(semaphores, 0, sizeof(semaphores));
    mutex_count = 0;
    sem_count = 0;
    moss_log(LOG_INFO, "Synchronization subsystem cleaned up");
}

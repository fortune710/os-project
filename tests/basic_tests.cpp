#include <assert.h>
#include "scheduler.h"
#include "mem.h"
#include "sync.h"

/* Run baseline integration checks for shared error codes and scheduler lifecycle behavior. */
int main(void) {
    assert(MOSS_OK == 0);
    assert(MOSS_EINVAL < 0);
    assert(MOSS_ENOMEM < 0);
    assert(MOSS_ENOTREADY < 0);
    assert(MOSS_ESTATE < 0);

    (void)sizeof(scheduler_process);
    (void)sizeof(process_control_block);
    (void)sizeof(mem_access_t);
    (void)sizeof(sync_access_t);

    assert(scheduler_create_process((const scheduler_process *)0) == MOSS_ENOTREADY);
    assert(scheduler_init(SCHEDULER_ALGORITHM_FCFS, 0) == MOSS_OK);
    assert(scheduler_init((scheduler_algorithm)99, 0) == MOSS_EINVAL);
    assert(scheduler_init(SCHEDULER_ALGORITHM_ROUND_ROBIN, 0) == MOSS_EINVAL);
    assert(scheduler_init(SCHEDULER_ALGORITHM_FCFS, 0) == MOSS_OK);

    scheduler_process first_scheduler_process_definition = {1, 0, 5, 0, 0, 0};
    scheduler_process second_scheduler_process_definition = {2, 0, 3, 0, 0, 0};

    assert(scheduler_create_process(&first_scheduler_process_definition) == MOSS_OK);
    assert(scheduler_create_process(&first_scheduler_process_definition) == MOSS_ESTATE);
    assert(scheduler_create_process(&second_scheduler_process_definition) == MOSS_OK);

    moss_pid_t scheduled_process_identifier = -1;
    assert(scheduler_schedule(&scheduled_process_identifier) == MOSS_OK);
    assert(scheduled_process_identifier == 1);

    assert(scheduler_tick(2) == MOSS_OK);

    scheduler_statistics global_scheduler_statistics = {0, 0, 0, 0};
    assert(scheduler_get_statistics(&global_scheduler_statistics) == MOSS_OK);
    assert(global_scheduler_statistics.current_time == 2);
    assert(global_scheduler_statistics.completed_count == 0);

    assert(scheduler_terminate_process(2) == MOSS_OK);
    assert(scheduler_terminate_process(99) == MOSS_EINVAL);

    assert(scheduler_tick(3) == MOSS_OK);
    assert(scheduler_get_statistics(&global_scheduler_statistics) == MOSS_OK);
    assert(global_scheduler_statistics.completed_count == 2);
    assert(scheduler_schedule(&scheduled_process_identifier) == MOSS_ESTATE);

    return 0;
}

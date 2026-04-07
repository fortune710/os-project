#include <assert.h>
#include "sched.h"
#include "mem.h"
#include "sync.h"

int main(void) {
    // Verify error codes are negative and success is zero
    assert(MOSS_OK == 0);
    assert(MOSS_EINVAL < 0);
    assert(MOSS_ENOMEM < 0);
    assert(MOSS_ENOTREADY < 0);
    assert(MOSS_ESTATE < 0);

    // Ensure headers compile together
    (void)sizeof(sched_process_t);
    (void)sizeof(mem_access_t);
    (void)sizeof(sync_access_t);

    return 0;
}

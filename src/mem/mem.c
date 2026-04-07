#include "mem.h"

moss_status_t mem_init(const mem_config_t *cfg) {
    (void)cfg;
    return MOSS_ENOTREADY;
}

moss_status_t mem_configure(const mem_config_t *cfg) {
    (void)cfg;
    return MOSS_ENOTREADY;
}

moss_status_t mem_access(const mem_access_t *access, uint64_t *out_physical_address, int *out_page_fault) {
    (void)access;
    (void)out_physical_address;
    (void)out_page_fault;
    return MOSS_ENOTREADY;
}

size_t mem_page_faults(void) {
    return 0;
}

moss_status_t mem_set_policy(mem_policy_t policy) {
    (void)policy;
    return MOSS_ENOTREADY;
}

moss_status_t mem_reset(void) {
    return MOSS_ENOTREADY;
}

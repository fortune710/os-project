#ifndef MOSS_MEM_H
#define MOSS_MEM_H

#include "moss.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MEM_FIFO = 0,
    MEM_LRU = 1,
    MEM_OPT = 2,
    MEM_WORKING_SET = 3
} mem_policy_t;

typedef struct {
    uint64_t logical_address;
    uint32_t access_size;
} mem_access_t;

typedef struct {
    size_t page_size;
    size_t logical_address_bits;
    size_t physical_frames;
    mem_policy_t policy;
} mem_config_t;

moss_status_t mem_init(const mem_config_t *cfg);
moss_status_t mem_configure(const mem_config_t *cfg);
moss_status_t mem_access(const mem_access_t *access, uint64_t *out_physical_address, int *out_page_fault);
size_t mem_page_faults(void);
moss_status_t mem_set_policy(mem_policy_t policy);
moss_status_t mem_reset(void);

#ifdef __cplusplus
}
#endif

#endif

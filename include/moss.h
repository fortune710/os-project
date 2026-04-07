#ifndef MOSS_COMMON_H
#define MOSS_COMMON_H

#include <stddef.h>
#include <stdint.h>

typedef int moss_status_t;
typedef int32_t moss_pid_t;
typedef int64_t moss_time_t;

typedef enum {
    MOSS_OK = 0,
    MOSS_EINVAL = -1,
    MOSS_ENOMEM = -2,
    MOSS_ENOTREADY = -3,
    MOSS_ESTATE = -4,
    MOSS_EPERM = -5
} moss_error_t;

#endif

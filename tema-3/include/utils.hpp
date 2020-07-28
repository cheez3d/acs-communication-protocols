#pragma once

#include <errno.h>
#include <error.h>

#define DIE(cond, msg, ...) \
    do { \
        if ((cond)) { \
            ::error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
        } \
    } while (0)

#ifndef TACC_GCC_COMPAT_H
#define TACC_GCC_COMPAT_H

#include <stdio.h>

#define PRIsz "zu"
#define init_io()
#define kill_io()   \
    fflush(stderr); \
    fflush(stdout);
#define tacc_assert(cond, ...) \
    if (!(cond)) {             \
        tacc_die(__VA_ARGS__); \
    }

#endif

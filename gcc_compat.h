#ifndef TACC_GCC_COMPAT_H
#define TACC_GCC_COMPAT_H

#include <stdio.h>

#define PRIsz "zu"

static inline void init_io(void) { }

static inline void kill_io(void) {
    fflush(stderr);
    fflush(stdout);
}

#endif

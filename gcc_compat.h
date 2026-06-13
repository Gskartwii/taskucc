#ifndef TACC_GCC_COMPAT_H
#define TACC_GCC_COMPAT_H

#include <stdio.h>

#define PRIsz "zu"
#define init_io()
#define kill_io() fflush(stderr); fflush(stdout);

#endif

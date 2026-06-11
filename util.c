#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "gcc_compat.h"

void tacc_die(char *err, ...) {
    va_list args;

    va_start(args, err);

    vfprintf(stderr, err, args);
    fputs("", stderr);
    kill_io();
    exit(1);
}

void tacc_assert(int cond, char *err, ...) {
    va_list args;
    if (!cond) {
        va_start(args, err);

        vfprintf(stderr, err, args);
        fputs("", stderr);
        kill_io();
        exit(1);
    }
    va_end(args);
}

void *tacc_malloc(size_t sz) {
    void *out;
    out = malloc(sz);
    tacc_assert(out != NULL, "failed to allocate %" PRIsz " bytes", sz);
    return out;
}

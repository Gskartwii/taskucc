#ifndef tacc_m2_shim
#define tacc_m2_shim

#include "util.h"
#define PRIsz "u"

#define init_io __init_io
#define kill_io __kill_io

static void
tacc_assert(enum tacc_assertion_class class, int cond, char *err, ...) {
    va_list args;
    va_start(args, err);

    if (cond) {
        return;
    }
    switch (class) {
    case ASSERT_ICE:
        fputs("Internal compiler error!", stderr);
        break;
    case ASSERT_DIAG:
        fputs("Diagnostic:", stderr);
        break;
    case ASSERT_TODO:
        fputs("TODO! Cannot continue.", stderr);
        break;
    }

    vfprintf(stderr, err, args);
    va_end(args);
    fputs("", stderr);
    kill_io();
    exit(1);
}

#endif

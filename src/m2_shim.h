#ifndef tacc_m2_shim
#define tacc_m2_shim

#define PRIsz "u"

#define init_io __init_io
#define kill_io __kill_io

static void tacc_assert(int cond, char *err, ...) {
    va_list args;
    va_start(args, err);

    if (cond) {
        return;
    }

    vfprintf(stderr, err, args);
    fputs("", stderr);
    kill_io();
    exit(1);
}

#endif

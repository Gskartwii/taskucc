#include "util.h"
#include "gcc_compat.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void tacc_die(char *err, ...) {
    va_list args;

    va_start(args, err);

    vfprintf(stderr, err, args);
    fputs("", stderr);
    kill_io();
    exit(1);
}

void *tacc_malloc(size_t sz) {
    void *out;
    out = malloc(sz);
    tacc_assert(out != NULL, "failed to allocate %" PRIsz " bytes", sz);
    return out;
}

void tacc_free(void *allocation) {
    tacc_assert(allocation != NULL, "free of NULL");
    free(allocation);
}

uint8_t tacc_hex_to_dec(char hex) {
    if (hex >= '0' && hex <= '9') {
        return (uint8_t) (hex - '0');
    }
    if (hex >= 'a' && hex <= 'f') {
        return (uint8_t) ((hex - 'a') + 10);
    }
    if (hex >= 'A' && hex <= 'F') {
        return (uint8_t) ((hex - 'A') + 10);
    }
    tacc_assert(0, "invalid hex character %x", (int) hex);
    return 0;
}

size_t tacc_sizeadj(size_t count, size_t size) {
#ifdef __M2__
    return count * size;
#else
    (void) size;
    return count;
#endif
}

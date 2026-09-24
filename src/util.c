#include "util.h"
#include "gcc_compat.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void tacc_die(enum tacc_assertion_class class, char *err, ...) {
    va_list args;

    va_start(args, err);

    fputs("\n", stderr);
    switch (class) {
    case ASSERT_ICE:
        fputs("Internal compiler error!\n", stderr);
        break;
    case ASSERT_DIAG:
        fputs("Diagnostic:\n", stderr);
        break;
    case ASSERT_TODO:
        fputs("TODO! Cannot continue.\n", stderr);
        break;
    }

    vfprintf(stderr, err, args);
    va_end(args);

    fputs("\n", stderr);
    kill_io();
    exit(1);
}

void *tacc_malloc(size_t sz) {
    void *out;
    out = malloc(sz);
    tacc_assert(
        ASSERT_ICE, out != NULL, "failed to allocate %" PRIsz " bytes", sz);
    return out;
}

void tacc_free(void *allocation) {
    /* on M2, free() is extremely slow; disable it by default */
#ifndef __M2__
    tacc_assert(ASSERT_ICE, allocation != NULL, "free of NULL");
    free(allocation);
#endif
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
    tacc_assert(ASSERT_DIAG, 0, "invalid hex character %x", (int) hex);
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

tacc_bool tacc_str_is_eq(char *a, char *b) {
    while (*a == *b) {
        if (*a == 0) {
            return 1;
        }
        a = a + 1;
        b = b + 1;
    }
    return 0;
}

size_t tacc_align_down(size_t x, size_t alignment_p2) {
    return x & ~((size_t) ((1 << alignment_p2) - 1));
}

size_t tacc_align_up(size_t x, size_t alignment_p2) {
    return (x + (size_t) (1 << alignment_p2) - 1) &
           ~((size_t) ((1 << alignment_p2) - 1));
}

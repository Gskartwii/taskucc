#include "binops.inc"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#define DECL_BINOP(output, left, right, op, suffix) \
    extern output binop_##suffix(left, right);

#define CHECK_BINOP(output, left, right, op, suffix)             \
    {                                                            \
        left l = (left) check_n_l;                               \
        right r = (right) check_n_r;                             \
        if (!r) {                                                \
            continue; /* avoid div by zero */                    \
        }                                                        \
        output expected = l op r;                                \
        if (binop_##suffix(l, r) != expected) {                  \
            printf("binop failed on (" #left ")%llu %s (" #right \
                   ")%llu -> " #output ": %llu != %llu\n",       \
                   (unsigned long long) check_n_l,               \
                   #op,                                          \
                   (unsigned long long) check_n_r,               \
                   (unsigned long long) binop_##suffix(l, r),    \
                   (unsigned long long) expected);               \
            ok = 0;                                              \
        }                                                        \
    }

#define BINOP_ACTION DECL_BINOP
BINOP_LIST
#undef BINOP_ACTION

#define BINOP_ACTION CHECK_BINOP

int main(void) {
    _Bool ok = 1;
    long long check_ns[] = {
        0,       1,       2,        3,        12,        15,
        16,      17,      31,       32,       33,        36,
        63,      64,      65,       -1,       -50,       255,
        128,     -256,    65535,    -32768,   32768,     32767,
        INT_MIN, INT_MAX, LONG_MIN, LONG_MAX, LLONG_MIN, LLONG_MAX,
    };

    for (size_t n_l = 0; n_l < sizeof(check_ns) / sizeof(check_ns[0]); n_l++) {
        for (size_t n_r = 0; n_r < sizeof(check_ns) / sizeof(check_ns[0]);
             n_r++) {
            long long check_n_l = check_ns[n_l];
            long long check_n_r = check_ns[n_r];
            BINOP_LIST
        }
    }

    return !ok;
}

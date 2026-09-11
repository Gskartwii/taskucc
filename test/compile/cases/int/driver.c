#include "casters.inc"
#include <limits.h>
#include <stdio.h>

extern int konst(void);

#define DECL_CASTER(from, to, n) \
    extern to cast_##n(from);    \
    extern to store_##n(from);
#define CHECK_CASTER(from, to, n)                                       \
    if (cast_##n((from) check_n) != ((to) (from) check_n)) {            \
        printf("caster failed on " #from " -> " #to ": %lld != %lld\n", \
               (long long) cast_##n((from) check_n),                    \
               (long long) ((to) (from) check_n));                      \
        ok = 0;                                                         \
    }                                                                   \
    if (store_##n((from) check_n) != ((to) (from) check_n)) {           \
        printf("store failed on " #from " -> " #to ": %lld != %lld\n",  \
               (long long) store_##n((from) check_n),                   \
               (long long) ((to) (from) check_n));                      \
        ok = 0;                                                         \
    }

#define CASTER_ACTION DECL_CASTER
CASTER_LIST
#undef CASTER_ACTION

#define CASTER_ACTION CHECK_CASTER

int main(void) {
    _Bool ok = 1;
    long long check_ns[] = {
        0,
        1,
        -1,
        -50,
        255,
        128,
        -256,
        65535,
        -32768,
        32768,
        32767,
        INT_MIN,
        INT_MAX,
        LONG_MIN,
        LONG_MAX,
        LLONG_MIN,
        LLONG_MAX,
    };

    if (konst() != 42) {
        printf("konst() != 42");
    }

    for (size_t n = 0; n < sizeof(check_ns) / sizeof(check_ns[0]); n++) {
        long long check_n = check_ns[n];
        CASTER_LIST
    }

    return !ok;
}

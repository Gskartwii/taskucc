#include "casters.inc"
#define MK_CASTER(from, to, n)        \
    to cast_##n(from x) { return x; } \
    to store_##n(from x) {            \
        from intermediate;            \
        intermediate = x;             \
        return intermediate;          \
    }

int konst(void) { return 42; }

#define CASTER_ACTION MK_CASTER
CASTER_LIST

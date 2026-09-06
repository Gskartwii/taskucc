#include "casters.inc"
#define MK_CASTER(from, to, n)        \
    to cast_##n(from x) { return x; }

int konst(void) { return 42; }

#define CASTER_ACTION MK_CASTER
CASTER_LIST

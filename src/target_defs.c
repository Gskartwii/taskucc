#include "target_defs.h"
#include "util.h"
#include <string.h>

struct tacc_target *tacc_target_new(char *desc) {
    struct tacc_target *target;

    target = tacc_malloc(sizeof(struct tacc_target));
    tacc_assert(!strcmp(desc, "x86_64-linux"), "unsupported target %s", desc);

    target->char_bit = 8;
    target->int_max = tacc_u64_new_from_u32(0x7FFFFFFF);
    target->uint_max = tacc_u64_new_from_u32(0xFFFFFFFF);
    target->long_max = tacc_u64_new();
    target->long_max->low = 0xFFFFFFFF;
    target->long_max->high = 0x7FFFFFFF;
    target->ulong_max = tacc_u64_new();
    target->ulong_max->low = 0xFFFFFFFF;
    target->ulong_max->high = 0xFFFFFFFF;
    target->llong_max = tacc_u64_clone(target->long_max);
    target->ullong_max = tacc_u64_clone(target->ulong_max);

    return target;
}

void tacc_target_free(struct tacc_target *target) {
    tacc_free(target->int_max);
    tacc_free(target->uint_max);
    tacc_free(target->long_max);
    tacc_free(target->ulong_max);
    tacc_free(target->llong_max);
    tacc_free(target->ullong_max);
    tacc_free(target);
}

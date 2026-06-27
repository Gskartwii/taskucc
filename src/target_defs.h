#ifndef TACC_TARGET_DEFS_H
#define TACC_TARGET_DEFS_H

#include "soft_u64.h"

struct tacc_target {
    int char_bit;
    struct tacc_u64 *int_max;
    struct tacc_u64 *uint_max;
    struct tacc_u64 *long_max;
    struct tacc_u64 *ulong_max;
    struct tacc_u64 *llong_max;
    struct tacc_u64 *ullong_max;
};

struct tacc_target *tacc_target_new(char *desc);
void tacc_target_free(struct tacc_target *free);

#endif

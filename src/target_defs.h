#ifndef TACC_TARGET_DEFS_H
#define TACC_TARGET_DEFS_H

#include "soft_u64.h"

struct tacc_int_type {
    size_t bit_width;
    struct tacc_u64 *min;
    struct tacc_u64 *max;
};

struct tacc_target {
    tacc_bool signed_char;
    struct tacc_int_type *schar;
    struct tacc_int_type *uchar;
    struct tacc_int_type *sshort;
    struct tacc_int_type *ushort;
    struct tacc_int_type *sint;
    struct tacc_int_type *uint;
    struct tacc_int_type *slong;
    struct tacc_int_type *ulong;
    struct tacc_int_type *sllong;
    struct tacc_int_type *ullong;
};

struct tacc_target *tacc_target_new(char *desc);
void tacc_target_free(struct tacc_target *free);

#endif

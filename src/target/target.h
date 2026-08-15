#ifndef TACC_TARGET_DEFS_H
#define TACC_TARGET_DEFS_H

#include "soft_u64.h"

struct tacc_int_type {
    size_t bit_width;
    size_t alignment_p2;
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
    struct tacc_int_type *bool_ty;
    struct {
        size_t bit_width;
        size_t alignment_p2;
    } pointer_ty;
};

struct tacc_target *tacc_target_new(void);
void tacc_target_free(struct tacc_target *free);

struct tacc_target_place_register {
    uint32_t reg;
};

#endif

#include "target/target.h"
#include "util.h"

static struct tacc_int_type *tacc_mk_twos_complement(size_t bit_size,
                                                     size_t alignment_p2,
                                                     tacc_bool is_signed) {
    struct tacc_int_type *ret;

    ret = tacc_malloc(sizeof(struct tacc_int_type));
    ret->bit_width = bit_size;
    ret->alignment_p2 = alignment_p2;
    ret->min = tacc_u64_new();
    ret->max = tacc_u64_new();

    ret->max->low = 1;
    tacc_u64_neg(ret->max, ret->max);
    tacc_u64_rsh_n(ret->max, ret->max, 64 - (int) bit_size);
    if (is_signed) {
        tacc_u64_rsh_n(ret->max, ret->max, 1);
        tacc_u64_add_u32(ret->min, ret->max, 1);
        tacc_u64_neg(ret->min, ret->min);
    }

    return ret;
}

struct tacc_target *tacc_target_new(void) {
    struct tacc_target *target;

    target = tacc_malloc(sizeof(struct tacc_target));

    target->signed_char = 0;
    target->schar = tacc_mk_twos_complement(8, 0, 1);
    target->uchar = tacc_mk_twos_complement(8, 0, 0);
    target->sshort = tacc_mk_twos_complement(16, 1, 1);
    target->ushort = tacc_mk_twos_complement(16, 1, 0);
    target->sint = tacc_mk_twos_complement(32, 2, 1);
    target->uint = tacc_mk_twos_complement(32, 2, 0);
    target->slong = tacc_mk_twos_complement(32, 2, 1);
    target->ulong = tacc_mk_twos_complement(32, 2, 0);
    target->sllong = tacc_mk_twos_complement(64, 3, 1);
    target->ullong = tacc_mk_twos_complement(64, 3, 0);
    target->bool_ty = tacc_mk_twos_complement(8, 0, 0);
    target->pointer_ty.alignment_p2 = 2;
    target->pointer_ty.bit_width = 32;

    return target;
}

static void tacc_free_int_type(struct tacc_int_type *ty) {
    tacc_free(ty->max);
    tacc_free(ty->min);
    tacc_free(ty);
}

void tacc_target_free(struct tacc_target *target) {
    tacc_free_int_type(target->schar);
    tacc_free_int_type(target->uchar);
    tacc_free_int_type(target->sshort);
    tacc_free_int_type(target->ushort);
    tacc_free_int_type(target->sint);
    tacc_free_int_type(target->uint);
    tacc_free_int_type(target->slong);
    tacc_free_int_type(target->ulong);
    tacc_free_int_type(target->sllong);
    tacc_free_int_type(target->ullong);
    tacc_free_int_type(target->bool_ty);
    tacc_free(target);
}

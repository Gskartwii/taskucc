#include "type.h"

tacc_bool tacc_type_kind_is_signed(enum tacc_type_kind kind,
                                   struct tacc_target *target) {
    switch (kind) {
    case TYK_CHAR:
        return target->signed_char;
    case TYK_SCHAR:
    case TYK_SSHORT:
    case TYK_SINT:
    case TYK_SLONG:
    case TYK_SLONGLONG:
        return 1;
    default:
        return 0;
    }
}

static struct tacc_int_type *tacc_target_int_type(struct tacc_target *target,
                                                  enum tacc_type_kind kind) {
    switch (kind) {
    case TYK_CHAR:
        if (target->signed_char) {
            return target->schar;
        } else {
            return target->uchar;
        }
    case TYK_UCHAR:
        return target->uchar;
    case TYK_SCHAR:
        return target->schar;
    case TYK_USHORT:
        return target->ushort;
    case TYK_SSHORT:
        return target->sshort;
    case TYK_UINT:
        return target->uint;
    case TYK_SINT:
        return target->sint;
    case TYK_ULONG:
        return target->ulong;
    case TYK_SLONG:
        return target->slong;
    case TYK_ULONGLONG:
        return target->ullong;
    case TYK_SLONGLONG:
        return target->sllong;
    default:
        tacc_assert(0, "todo: non-integral type requested");
        return 0;
    }
}

struct tacc_u64 *tacc_type_max_val(struct tacc_target *target,
                                   enum tacc_type_kind kind) {
    struct tacc_int_type *ty;

    ty = tacc_target_int_type(target, kind);

    return ty->max;
}
struct tacc_u64 *tacc_type_min_val(struct tacc_target *target,
                                   enum tacc_type_kind kind) {
    struct tacc_int_type *ty;

    ty = tacc_target_int_type(target, kind);

    return ty->min;
}

size_t tacc_type_bit_width(struct tacc_target *target,
                           enum tacc_type_kind kind) {
    struct tacc_int_type *ty;

    ty = tacc_target_int_type(target, kind);

    return ty->bit_width;
}

tacc_bool tacc_type_is_subset(enum tacc_type_kind subset,
                              enum tacc_type_kind superset,
                              struct tacc_target *target) {
    struct tacc_u64 *max_val_superset;
    struct tacc_u64 *min_val_superset;
    struct tacc_u64 *max_val_subset;
    struct tacc_u64 *min_val_subset;

    if (tacc_type_kind_is_signed(subset, target)) {
        if (!tacc_type_kind_is_signed(superset, target)) {
            return 0;
        }
        min_val_subset = tacc_type_min_val(target, subset);
        min_val_superset = tacc_type_min_val(target, superset);
        if (tacc_u64_slt(min_val_superset, min_val_subset)) {
            return 0;
        }
    }

    max_val_subset = tacc_type_max_val(target, subset);
    max_val_superset = tacc_type_max_val(target, superset);
    if (tacc_u64_ugt(max_val_superset, max_val_subset)) {
        return 0;
    }
    return 1;
}

enum tacc_int_rank tacc_type_rank(enum tacc_type_kind kind) {
    switch (kind) {
    case TYK_BOOL:
        return IRANK_BOOL;
    case TYK_CHAR:
    case TYK_UCHAR:
    case TYK_SCHAR:
        return IRANK_CHAR;
    case TYK_USHORT:
    case TYK_SSHORT:
        return IRANK_SHORT;
    case TYK_UINT:
    case TYK_SINT:
        return IRANK_INT;
    case TYK_ULONG:
    case TYK_SLONG:
        return IRANK_LONG;
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
        return IRANK_LLONG;
    default:
        tacc_assert(0, "cannot compute rank for non-integral type");
        return 0;
    }
}

enum tacc_type_kind tacc_type_to_unsigned(enum tacc_type_kind kind) {
    switch (kind) {
    case TYK_CHAR:
    case TYK_UCHAR:
    case TYK_SCHAR:
        return TYK_UCHAR;
    case TYK_USHORT:
    case TYK_SSHORT:
        return TYK_USHORT;
    case TYK_UINT:
    case TYK_SINT:
        return TYK_UINT;
    case TYK_ULONG:
    case TYK_SLONG:
        return TYK_ULONG;
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
        return TYK_ULONGLONG;
    case TYK_BOOL:
        return TYK_BOOL;
    default:
        tacc_assert(0, "can't convert non-integral type kind to unsigned");
        return 0;
    }
}

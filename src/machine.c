#include "machine.h"
#include "soft_u64.h"
#include "type.h"
#include <string.h>

struct tacc_val *tacc_val_new(void) {
    struct tacc_val *val = tacc_malloc(sizeof(struct tacc_val));

    val->type_kind = TYK_VOID;
    val->value.int_value = NULL;
    val->compound_type = NULL;

    return val;
}

struct tacc_val *tacc_val_clone(struct tacc_val *orig_val) {
    struct tacc_val *val = tacc_malloc(sizeof(struct tacc_val));

    val->type_kind = orig_val->type_kind;
    val->compound_type = orig_val->compound_type;

    if (tacc_val_is_integral(orig_val)) {
        val->value.int_value = tacc_u64_clone(orig_val->value.int_value);
    } else {
        tacc_assert(0, "non-integral vals");
    }

    return val;
}

tacc_bool tacc_val_is_integral(struct tacc_val *val) {
    switch (val->type_kind) {
    case TYK_CHAR:
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
        return 1;
    default:
        return 0;
    }
}

tacc_bool tacc_val_is_signed(struct tacc_val *val, struct tacc_target *target) {
    return tacc_type_kind_is_signed(val->type_kind, target);
}

tacc_bool tacc_val_is_scalar(struct tacc_val *val) {
    switch (val->type_kind) {
    case TYK_CHAR:
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_FLOAT:
    case TYK_DOUBLE:
    case TYK_LONGDOUBLE:
    case TYK_BOOL:
        return 1;
    case TYK_VOID:
        return 0;
    case TYK_COMPOUND:
        return (val->compound_type->kind == TYC_PTR) ||
               (val->compound_type->kind == TYC_ENUM);
    }
    return 0;
}

tacc_bool tacc_val_is_arithmetic(struct tacc_val *val) {
    switch (val->type_kind) {
    case TYK_CHAR:
    case TYK_UCHAR:
    case TYK_SCHAR:
    case TYK_USHORT:
    case TYK_SSHORT:
    case TYK_UINT:
    case TYK_SINT:
    case TYK_ULONG:
    case TYK_SLONG:
    case TYK_ULONGLONG:
    case TYK_SLONGLONG:
    case TYK_FLOAT:
    case TYK_DOUBLE:
    case TYK_LONGDOUBLE:
    case TYK_BOOL:
        return 1;
    case TYK_VOID:
        return 0;
    case TYK_COMPOUND:
        return val->compound_type->kind == TYC_ENUM;
    }
    return 0;
}

tacc_bool tacc_val_is_truthy(struct tacc_val *val) {
    if (tacc_val_is_integral(val)) {
        return !tacc_u64_is_zero(val->value.int_value);
    }
    tacc_assert(0, "todo: non-integral values");
    return 0;
}

static void tacc_val_convert(struct tacc_val *val,
                             enum tacc_type_kind into,
                             struct tacc_target *target) {
    if (tacc_type_is_subset(val->type_kind, into, target)) {
        /* already fits into destination type */
        return;
    }
    if (tacc_type_kind_is_signed(into, target)) {
        tacc_u64_sext(val->value.int_value,
                      val->value.int_value,
                      (int) tacc_type_bit_width(target, into));
        return;
    }
    tacc_u64_zext(val->value.int_value,
                  val->value.int_value,
                  (int) tacc_type_bit_width(target, into));
}

void tacc_val_usual_arithmetic_conversions(struct tacc_val *a,
                                           struct tacc_val *b,
                                           struct tacc_target *target) {
    enum tacc_type_kind a_type;
    enum tacc_type_kind b_type;
    enum tacc_int_rank a_rank;
    enum tacc_int_rank b_rank;
    enum tacc_type_kind common_type;

    a_type = a->type_kind;
    b_type = b->type_kind;

    tacc_assert(tacc_val_is_integral(a) && tacc_val_is_integral(b),
                "TODO: arith conversions for non-integral types");

    if (a_type == b_type) {
        return;
    }

    a_rank = tacc_type_rank(a_type);
    b_rank = tacc_type_rank(b_type);

    if (tacc_val_is_signed(a, target) == tacc_val_is_signed(b, target)) {
        /* same signedness => convert to the higher-ranked of the types */
        if (a_rank <= b_rank) {
            tacc_val_convert(a, b_type, target);
        } else {
            tacc_val_convert(b, a_type, target);
        }
        return;
    }
    if (!tacc_val_is_signed(a, target) && (b_rank <= a_rank)) {
        tacc_val_convert(b, a_type, target);
    } else if (!tacc_val_is_signed(b, target) && (a_rank <= b_rank)) {
        tacc_val_convert(a, b_type, target);
    } else if (tacc_val_is_signed(a, target) &&
               tacc_type_is_subset(b_type, a_type, target)) {
        tacc_val_convert(b, a_type, target);
    } else if (tacc_val_is_signed(b, target) &&
               tacc_type_is_subset(a_type, b_type, target)) {
        tacc_val_convert(a, b_type, target);
    } else {
        if (tacc_val_is_signed(b, target)) {
            common_type = b_type;
        } else {
            common_type = a_type;
        }
        common_type = tacc_type_to_unsigned(common_type);
        tacc_val_convert(a, common_type, target);
    }
}

tacc_bool tacc_val_is_eq(struct tacc_val *a, struct tacc_val *b) {
    tacc_assert(tacc_val_is_integral(a) && tacc_val_is_integral(b),
                "TODO: non-integral eq");
    return tacc_u64_eq(a->value.int_value, b->value.int_value);
}

struct tacc_val *tacc_val_from_int(int value,
                                   enum tacc_type_kind kind,
                                   struct tacc_target *target) {
    struct tacc_val *val;

    val = tacc_val_new();
    val->type_kind = kind;
    val->value.int_value = tacc_u64_new_from_u32((uint32_t) value);
    if (tacc_val_is_signed(val, target) && (value < 0)) {
        val->value.int_value->high = 0xFFFFFFFF;
    }

    return val;
}

void tacc_val_free(struct tacc_val *val) {
    if (tacc_val_is_integral(val)) {
        tacc_free(val->value.int_value);
    }
    tacc_free(val);
}

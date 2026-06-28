#include "machine.h"
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

void tacc_val_free(struct tacc_val *val) {
    if (tacc_val_is_integral(val)) {
        tacc_free(val->value.int_value);
    }
    tacc_free(val);
}

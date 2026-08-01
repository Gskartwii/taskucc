#include "type.h"
#include "dynarray.h"

MK_DYNARRAY_OVER(tacc_compound_type_list,
                 tacc_compound_type_list_entry,
                 struct tacc_compound_type *,
                 tacc_compound_type_list_new,
                 tacc_compound_type_list_init,
                 tacc_compound_type_list_get,
                 tacc_compound_type_list_push,
                 tacc_compound_type_list_pop,
                 tacc_compound_type_list_len,
                 tacc_compound_type_free,
                 tacc_compound_type_list_free)

MK_DYNARRAY_OVER(tacc_type_list,
                 tacc_type_list_entry,
                 struct tacc_type *,
                 tacc_type_list_new,
                 tacc_type_list_init,
                 tacc_type_list_get,
                 tacc_type_list_push,
                 tacc_type_list_pop,
                 tacc_type_list_len,
                 tacc_type_free,
                 tacc_type_list_free)

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

void tacc_type_free(struct tacc_type *type) {
    if (type->kind == TYK_COMPOUND) {
        if (type->extra->kind != TYC_STRUCT && type->extra->kind != TYC_UNION &&
            type->extra->kind != TYC_ENUM) {
            tacc_compound_type_free(type->extra);
        }
    }

    if (type->derived_ptr != NULL) {
        tacc_type_free(type->derived_ptr);
    }
    tacc_type_list_free(type->derived_array_types);
    tacc_free(type->derived_array_types);
    tacc_type_list_free(type->derived_func_types);
    tacc_free(type->derived_func_types);
    tacc_free(type);
}

void tacc_compound_type_free(struct tacc_compound_type *type) {
    switch (type->kind) {
    case TYC_PTR:
    case TYC_ARRAY:
    case TYC_ARRAY_FLEX:
    case TYC_FN:
        break;
    case TYC_STRUCT:
    case TYC_UNION:
    case TYC_ENUM:
        tacc_assert(0, "TODO: struct/union/enum type");
        break;
    }
    tacc_free(type);
}

struct tacc_type *tacc_type_new(void) {
    struct tacc_type *type;

    type = tacc_malloc(sizeof(struct tacc_type));
    type->kind = TYK_SINT;
    type->derived_array_types = tacc_type_list_new();
    type->derived_func_types = tacc_type_list_new();
    type->derived_ptr = NULL;

    return type;
}

struct tacc_compound_type *tacc_compound_type_new(void) {
    struct tacc_compound_type *type;

    type = tacc_malloc(sizeof(struct tacc_compound_type));
    type->kind = TYC_PTR;
    type->name = NULL;

    return type;
}

struct tacc_array_type *tacc_array_type_new(void) {
    struct tacc_array_type *type;

    type = tacc_malloc(sizeof(struct tacc_array_type));
    type->element_type = NULL;
    type->dimension = NULL;

    return type;
}

struct tacc_function_type *tacc_function_type_new(void) {
    struct tacc_function_type *type;

    type = tacc_malloc(sizeof(struct tacc_function_type));
    type->param_types = NULL;
    type->param_list_kind = FUNCPARAM_LIST;
    type->return_type = NULL;

    return type;
}

struct tacc_type *tacc_get_basic_type(struct tacc_type_list *basic_types,
                                      enum tacc_type_kind kind) {
    struct tacc_type_list_entry *ty_entry;
    size_t i;

    for (i = 0; i < tacc_type_list_len(basic_types); i = i + 1) {
        ty_entry = tacc_type_list_get(basic_types, i);
        if (ty_entry->content->kind == kind) {
            return ty_entry->content;
        }
    }
    tacc_assert(0, "couldn't find registred type for basic type");
    return NULL;
}

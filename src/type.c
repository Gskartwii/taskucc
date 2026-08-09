#include "type.h"
#include "dynarray.h"

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

MK_DYNHASH_OVER(tacc_type_map,
                name->string,
                tacc_type_map_entry,
                struct tacc_type *,
                tacc_type_map_new,
                tacc_type_map_init,
                tacc_type_map_get,
                tacc_type_map_insert,
                tacc_type_map_count,
                tacc_type_free,
                tacc_type_map_free)

MK_DYNARRAY_OVER(tacc_field_list,
                 tacc_field_list_entry,
                 struct tacc_field *,
                 tacc_field_list_new,
                 tacc_field_list_init,
                 tacc_field_list_get,
                 tacc_field_list_push,
                 tacc_field_list_pop,
                 tacc_field_list_len,
                 tacc_field_free,
                 tacc_field_list_free)

tacc_bool tacc_type_kind_is_signed(enum tacc_type_kind kind) {
    switch (kind) {
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

size_t tacc_type_size(struct tacc_target *target, struct tacc_type *type) {
    struct tacc_int_type *ty;

    switch (type->kind) {
    case TYK_FLOAT:
    case TYK_DOUBLE:
    case TYK_LONGDOUBLE:
        tacc_assert(0, "TODO: floating point type size");
        return 0;
    case TYK_VOID:
        tacc_assert(0, "cannot take size of void");
        return 0;
    case TYK_PTR:
        return target->pointer_ty.bit_width >> 3;
    case TYK_STRUCT:
        return type->extra.structure->size;
    case TYK_UNION:
        return type->extra.onion->size;
    case TYK_ENUM:
        return tacc_type_size(target, type->extra.enumeration->underlying_type);
    case TYK_ARRAY:
        tacc_assert(type->extra.array->dimension->high == 0,
                    "TODO: array too large for sizeof");
        /* TODO: overflow in multiplication? */
        return tacc_type_size(target, type->extra.array->element_type) *
               type->extra.array->dimension->low;
    case TYK_INCOMPLETE_ARRAY:
        tacc_assert(0, "cannot take size of incomplete array type");
        return 0;
    case TYK_VLA:
        tacc_assert(0, "cannot take constant size of VLA.");
        return 0;
    case TYK_DECAYING_VLA:
        tacc_assert(0, "cannot take constant size of decaying VLA.");
        return 0;
    case TYK_FN:
        tacc_assert(0, "cannot take size of function");
        return 0;
    default:
        ty = tacc_target_int_type(target, type->kind);
        break;
    }

    return ty->bit_width >> 3;
}

size_t tacc_type_alignment_p2(struct tacc_target *target,
                              struct tacc_type *type) {
    struct tacc_int_type *ty;

    switch (type->kind) {
    case TYK_FLOAT:
    case TYK_DOUBLE:
    case TYK_LONGDOUBLE:
        tacc_assert(0, "TODO: floating point type alignment_p2");
        return 0;
    case TYK_VOID:
        tacc_assert(0, "cannot take alignment of void");
        return 0;
    case TYK_PTR:
        return target->pointer_ty.alignment_p2;
    case TYK_STRUCT:
        return type->extra.structure->alignment_p2;
    case TYK_UNION:
        return type->extra.onion->alignment_p2;
    case TYK_ENUM:
        return tacc_type_alignment_p2(target,
                                      type->extra.enumeration->underlying_type);
    case TYK_ARRAY:
    case TYK_INCOMPLETE_ARRAY:
    case TYK_VLA:
    case TYK_DECAYING_VLA:
        return tacc_type_alignment_p2(target, type->extra.array->element_type);
    case TYK_FN:
        tacc_assert(0, "cannot take alignment of function");
        return 0;
    default:
        ty = tacc_target_int_type(target, type->kind);
        break;
    }

    return ty->alignment_p2;
}

tacc_bool tacc_type_is_subset(enum tacc_type_kind subset,
                              enum tacc_type_kind superset,
                              struct tacc_target *target) {
    struct tacc_u64 *max_val_superset;
    struct tacc_u64 *min_val_superset;
    struct tacc_u64 *max_val_subset;
    struct tacc_u64 *min_val_subset;

    if (tacc_type_kind_is_signed(subset)) {
        if (!tacc_type_kind_is_signed(superset)) {
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
    switch (type->kind) {
    case TYK_ARRAY:
    case TYK_INCOMPLETE_ARRAY:
    case TYK_VLA:
    case TYK_DECAYING_VLA:
        tacc_array_type_free(type->extra.array);
        break;
    case TYK_FN:
        tacc_function_type_free(type->extra.function);
        break;
    default:
        break;
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

struct tacc_type *tacc_type_new(void) {
    struct tacc_type *type;

    type = tacc_malloc(sizeof(struct tacc_type));
    type->kind = TYK_SINT;
    type->derived_array_types = tacc_type_list_new();
    type->derived_func_types = tacc_type_list_new();
    type->derived_ptr = NULL;

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
    type->is_vararg = 0;
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

tacc_bool tacc_type_kind_is_integral(enum tacc_type_kind type_kind) {
    switch (type_kind) {
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
    case TYK_BOOL:
    case TYK_ENUM:
        return 1;
    default:
        return 0;
    }
}

tacc_bool tacc_type_is_integral(struct tacc_type *type) {
    if (tacc_type_kind_is_integral(type->kind)) {
        return 1;
    }
    return 0;
}

tacc_bool tacc_type_kind_is_scalar(enum tacc_type_kind type_kind) {
    switch (type_kind) {
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
    case TYK_PTR:
    case TYK_ENUM:
        return 1;
    default:
        return 0;
    }
    return 0;
}

tacc_bool tacc_type_is_scalar(struct tacc_type *type) {
    return tacc_type_kind_is_scalar(type->kind);
}

void tacc_array_type_free(struct tacc_array_type *array_type) {
    if (array_type->dimension != NULL) {
        tacc_free(array_type->dimension);
    }
    tacc_free(array_type);
}

void tacc_function_type_free(struct tacc_function_type *function_type) {
    /*
     * do not use tacc_type_list_free, that would free the borrowed contents.
     * Only free the list.
     */
    tacc_free(function_type->param_types->list->buffer);
    tacc_free(function_type->param_types);
    tacc_free(function_type);
}

static struct tacc_type *tacc_mk_basic_type(enum tacc_type_kind kind) {
    struct tacc_type *type;

    type = tacc_type_new();
    type->kind = kind;

    return type;
}

struct tacc_type *tacc_type_to_pointer(struct tacc_type *base_type,
                                       size_t indirection_level) {
    struct tacc_type *ty;
    size_t i;

    ty = base_type;
    for (i = 0; i < indirection_level; i = i + 1) {
        if (ty->derived_ptr != NULL) {
            ty = ty->derived_ptr;
            continue;
        }
        ty->derived_ptr = tacc_type_new();
        ty->derived_ptr->kind = TYK_PTR;
        ty->derived_ptr->extra.pointee = ty;
        ty = ty->derived_ptr;
    }

    return ty;
}

void tacc_gen_basic_types(struct tacc_type_list *into) {
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_SCHAR));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_UCHAR));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_SSHORT));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_USHORT));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_SINT));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_UINT));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_SLONG));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_ULONG));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_SLONGLONG));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_ULONGLONG));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_FLOAT));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_DOUBLE));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_LONGDOUBLE));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_BOOL));
    tacc_type_list_push(into, tacc_mk_basic_type(TYK_VOID));
}

struct tacc_enumeration_type *tacc_enumeration_type_new(void) {
    struct tacc_enumeration_type *type;

    type = tacc_malloc(sizeof(struct tacc_enumeration_type));
    type->underlying_type = NULL;

    return type;
}

struct tacc_struct_type *tacc_struct_type_new(void) {
    struct tacc_struct_type *type;

    type = tacc_malloc(sizeof(struct tacc_struct_type));
    type->alignment_p2 = 0;
    type->size = 0;
    type->fields = tacc_field_list_new();

    return type;
}

struct tacc_union_type *tacc_union_type_new(void) {
    struct tacc_union_type *type;

    type = tacc_malloc(sizeof(struct tacc_union_type));
    type->alignment_p2 = 0;
    type->size = 0;
    type->fields = tacc_field_list_new();

    return type;
}

struct tacc_field *tacc_field_new(void) {
    struct tacc_field *type;

    type = tacc_malloc(sizeof(struct tacc_field));
    type->type = NULL;
    type->name = NULL;

    type->is_bitfield = 0;
    type->bit_offset = 0;
    type->bit_width = 0;
    type->offset = 0;

    return type;
}

void tacc_field_free(struct tacc_field *field) {
    if (field->name != NULL) {
        tacc_dynstring_free(field->name);
    }
    tacc_free(field);
}

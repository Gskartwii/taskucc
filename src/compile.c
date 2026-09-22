#include "compile.h"
#include "codegen.h"
#include "decl.h"
#include "expr.h"
#include "machine.h"
#include "string_list.h"
#include "target/codegen.h"
#include "type.h"
#include "util.h"
#include <stdarg.h>

static void tacc_ident_free(uint32_t ident) { TACC_UNUSED(ident); }

static void tacc_global_object_free(struct tacc_global_object *obj) {
    if (obj->is_enumerator) {
        tacc_val_free(obj->extra.enumerator_value);
    }
    tacc_free(obj);
}

MK_DYNARRAY_OVER(tacc_ident_list,
                 tacc_ident_list_entry,
                 uint32_t,
                 tacc_ident_list_new,
                 tacc_ident_list_init,
                 tacc_ident_list_get,
                 tacc_ident_list_push,
                 tacc_ident_list_pop,
                 tacc_ident_list_len,
                 tacc_ident_free,
                 tacc_ident_list_free)

MK_DYNHASH_OVER_U32(tacc_global_object_map,
                    name_ref,
                    tacc_global_object_map_entry,
                    struct tacc_global_object *,
                    tacc_global_object_map_new,
                    tacc_global_object_map_init,
                    tacc_global_object_map_get,
                    tacc_global_object_map_insert,
                    tacc_global_object_map_fill_count,
                    tacc_global_object_free,
                    tacc_global_object_map_free)

struct tacc_global_object *tacc_global_object_new(void) {
    struct tacc_global_object *obj;

    obj = tacc_malloc(sizeof(struct tacc_global_object));
    obj->is_enumerator = 0;
    obj->name_ref = 0;

    return obj;
}

static void tacc_compile_add_global_object(struct tacc_compiler *compiler,
                                           struct tacc_global_object *obj) {
    struct tacc_global_object *old_object;
    old_object = tacc_compile_resolve_global(compiler, obj->name_ref);
    if (old_object != NULL) {
        tacc_assert(ASSERT_DIAG,
                    old_object->is_enumerator && !obj->is_enumerator,
                    "%s redefined",
                    tacc_dynstring_as_str(
                        tacc_compile_get_name(compiler, obj->name_ref)));
        tacc_assert(ASSERT_DIAG,
                    tacc_type_is_compatible(old_object->extra.obj_type,
                                            obj->extra.obj_type),
                    "%s redefined with incompatible type",
                    tacc_dynstring_as_str(
                        tacc_compile_get_name(compiler, obj->name_ref)));
    }
    tacc_global_object_map_insert(compiler->global_objects, obj);
}

void tacc_compile_output_directive(struct tacc_compiler *compiler,
                                   char *directive_fmt,
                                   ...) {
    va_list va;

    TACC_UNUSED(compiler);

    va_start(va, directive_fmt);
    printf("\n\t.");
    vprintf(directive_fmt, va);
    va_end(va);
}

void tacc_compile_output(struct tacc_compiler *compiler, char *fmt, ...) {
    va_list va;

    TACC_UNUSED(compiler);

    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
}

static void tacc_compile_output_int(struct tacc_compiler *compiler,
                                    struct tacc_u64 *int_val,
                                    size_t bits) {
    size_t i;
    struct tacc_u64 aux;

    tacc_compile_output_directive(compiler, "byte ");

    for (i = 0; i < bits; i = i + 8) {
        if (i != 0) {
            tacc_compile_output(compiler, ", ");
        }
        tacc_u64_rsh_n(&aux, int_val, (int) i);
        tacc_compile_output(compiler, "%u", (unsigned) (aux.low & 0xFF));
    }
}

static void tacc_compile_val(struct tacc_compiler *compiler,
                             struct tacc_val *val,
                             struct tacc_string *name) {
    size_t bits;
    size_t alignment;

    tacc_compile_output_directive(compiler, "data");
    tacc_compile_output_directive(
        compiler, "global %s", tacc_dynstring_as_str(name));
    tacc_compile_output_directive(
        compiler, "type %s, %%object", tacc_dynstring_as_str(name));

    if (tacc_type_is_integral(val->type)) {
        bits = tacc_type_bit_width(val->type);
        alignment = tacc_type_alignment_p2(val->type);

        tacc_compile_output_directive(compiler, "p2align %d", (int) alignment);
        tacc_compile_output(compiler, "\n%s:", tacc_dynstring_as_str(name));

        tacc_compile_output_int(compiler, val->value.int_value, bits);
        return;
    }

    switch (val->type->kind) {
    case TYK_FLOAT:
        tacc_assert(ASSERT_TODO, 0, "float support");
        return;
    case TYK_DOUBLE:
        tacc_assert(ASSERT_TODO, 0, "float support");
        return;
    case TYK_LONGDOUBLE:
        tacc_assert(ASSERT_TODO, 0, "float support");
        return;
    case TYK_VOID:
        tacc_assert(ASSERT_TODO, 0, "cannot output value of void type");
        return;
    case TYK_PTR:
        tacc_assert(ASSERT_TODO, 0, "compile ptr constant");
        break;
    case TYK_STRUCT:
        tacc_assert(ASSERT_TODO, 0, "compile struct constant");
        break;
    case TYK_UNION:
        tacc_assert(ASSERT_TODO, 0, "compile union constant");
        break;
    case TYK_ENUM:
        tacc_assert(ASSERT_TODO, 0, "compile enum constant");
        break;
    case TYK_ARRAY:
        tacc_assert(ASSERT_TODO, 0, "compile array constant");
        break;
    case TYK_FN:
        tacc_assert(ASSERT_TODO, 0, "compile fn constant (?)");
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "unexpected type kind");
        return;
    }
}

static void tacc_compile_data(struct tacc_compiler *compiler,
                              struct tacc_type *for_type,
                              struct tacc_string *name,
                              struct tacc_initializer *initializer) {
    struct tacc_sub_initializer_list_entry *entry;
    struct tacc_expr *expr;
    struct tacc_val *val;

    if (tacc_type_is_scalar(for_type)) {
        if (initializer == NULL) {
            val = tacc_val_zero(for_type);
        } else {
            if (!initializer->plain_expr) {
                tacc_assert(
                    ASSERT_DIAG,
                    tacc_sub_initializer_list_len(initializer->value.list) == 1,
                    "multiple initializers for a scalar");
                entry =
                    tacc_sub_initializer_list_get(initializer->value.list, 0);
                tacc_assert(ASSERT_DIAG,
                            entry->content->designator_kind == DESIGNATOR_NONE,
                            "designator used on scalar");
                tacc_assert(ASSERT_DIAG,
                            entry->content->value->plain_expr,
                            "nested braced initializers for scalar");
                expr = entry->content->value->value.expr;
            } else {
                expr = initializer->value.expr;
            }
            val = tacc_expr_const_eval(
                expr, compiler->target, compiler->basic_types);
            tacc_val_convert(val, for_type);
            val->type = for_type;
        }
        tacc_compile_val(compiler, val, name);
        tacc_val_free(val);
        return;
    }
    tacc_assert(ASSERT_TODO, 0, "non-scalar data");
}

static struct tacc_type *tacc_eval_enumerators(
    struct tacc_compiler *compiler, struct tacc_enumerator_list *enumerators) {
    struct tacc_u64 counter;
    size_t i;
    struct tacc_enumerator_list_entry *entry;
    struct tacc_val *val;
    tacc_bool use_negative;

    use_negative = 0;
    tacc_u64_zero(&counter);

    for (i = 0; i < tacc_enumerator_list_len(enumerators); i = i + 1) {
        entry = tacc_enumerator_list_get(enumerators, i);
        if (entry->content->value != NULL) {
            val = tacc_expr_const_eval(
                entry->content->value, compiler->target, compiler->basic_types);
            tacc_assert(ASSERT_DIAG,
                        tacc_val_is_integral(val),
                        "enumerator must be integer constant");
            if (tacc_val_is_negative(val)) {
                use_negative = 1;
            }
            tacc_assert(ASSERT_DIAG,
                        tacc_u64_sge(val->value.int_value,
                                     compiler->target->sint->min) &&
                            tacc_u64_sle(val->value.int_value,
                                         compiler->target->sint->max),
                        "enumerator value out of range");
        } else {
            tacc_assert(ASSERT_DIAG,
                        counter.low != 0xFFFFFFFF || counter.high != 0xFFFFFFFF,
                        "enumerator overflow when implicitly incrementing");
            tacc_u64_add_u32(&counter, &counter, 1);
        }
    }

    if (use_negative) {
        return tacc_get_basic_type(compiler->basic_types, TYK_SINT);
    }
    return tacc_get_basic_type(compiler->basic_types, TYK_UINT);
}

static void
tacc_type_adjust_function(struct tacc_compiler *compiler,
                          struct tacc_function_type *ty,
                          struct tacc_function_declarator *declarator) {
    size_t i;
    struct tacc_function_param_list_entry *entry;
    struct tacc_type *param_type;

    ty->is_vararg = 0;

    switch (declarator->param_list_kind) {
    case FUNCPARAM_LIST_VARARG:
        ty->is_vararg = 1;
        tacc_assert(
            ASSERT_DIAG,
            tacc_function_param_list_len(declarator->param_list.modern_params) >
                0,
            "function with ... in parameter list without other parameters");
        /* fallthrough */
    case FUNCPARAM_LIST:
        ty->param_types = tacc_type_list_new();
        for (i = 0; i < tacc_function_param_list_len(
                            declarator->param_list.modern_params);
             i = i + 1) {
            entry = tacc_function_param_list_get(
                declarator->param_list.modern_params, i);
            param_type =
                tacc_type_from_decl_type(compiler, entry->content->base_type);
            param_type = tacc_type_adjust_from_declarator(
                compiler, param_type, entry->content->decl);
            param_type = tacc_type_normalize_function_param(
                compiler->target->pointer_ty, param_type);
            tacc_type_list_push(ty->param_types, param_type);
        }
        break;
    case FUNCPARAM_VOID:
        /* function specified to take no parameters */
        ty->param_types = tacc_type_list_new();
        break;
    case FUNCPARAM_EMPTY_LIST:
        /* function type declarator with unspecified parameter types */
        ty->param_types = NULL;
        break;
    case FUNCPARAM_OLD_STYLE_LIST:
        ty->param_types = NULL;
        break;
    }
}

struct tacc_type *
tacc_type_adjust_from_declarator(struct tacc_compiler *compiler,
                                 struct tacc_type *base_type,
                                 struct tacc_declarator *declarator) {
    struct tacc_declarator *curr_declarator;
    struct tacc_type *curr_type;
    struct tacc_type *sub_type;
    struct tacc_val *dimension;

    curr_declarator = declarator;
    curr_type = base_type;

    while (1) {
        if (curr_declarator->kind == DECLARATOR_PLAIN ||
            curr_declarator->kind == DECLARATOR_ABSTRACT) {
            return tacc_type_to_pointer(compiler->target->pointer_ty,
                                        curr_type,
                                        curr_declarator->indirection_level);
        }
        if (curr_declarator->kind == DECLARATOR_SUB) {
            curr_type =
                tacc_type_to_pointer(compiler->target->pointer_ty,
                                     curr_type,
                                     curr_declarator->indirection_level);
            curr_declarator = curr_declarator->extra.sub_declarator;
            continue;
        }
        if (curr_declarator->kind == DECLARATOR_ARRAY) {
            sub_type = tacc_type_to_pointer(compiler->target->pointer_ty,
                                            curr_type,
                                            curr_declarator->indirection_level);
            curr_type = tacc_type_new();
            curr_type->extra.array = tacc_array_type_new();
            curr_type->extra.array->element_type = sub_type;
            if (curr_declarator->extra.arr_decl->array_dim_kind ==
                ARRAYDIM_UNSPECIFIED) {
                curr_type->kind = TYK_INCOMPLETE_ARRAY;
            } else if (curr_declarator->extra.arr_decl->array_dim_kind ==
                       ARRAYDIM_UNSPECIFIED_VLA) {
                curr_type->kind = TYK_DECAYING_VLA;
            } else {
                dimension = tacc_expr_const_eval(
                    curr_declarator->extra.arr_decl->dim_expr,
                    compiler->target,
                    compiler->basic_types);
                if (dimension != NULL) {
                    curr_type->kind = TYK_ARRAY;
                    tacc_assert(ASSERT_DIAG,
                                tacc_val_is_integral(dimension),
                                "array dimension must be an integer");
                    tacc_assert(ASSERT_DIAG,
                                !tacc_val_is_negative(dimension),
                                "array dimension must be nonnegative");
                    tacc_val_convert(
                        dimension,
                        tacc_get_basic_type(compiler->basic_types, TYK_UINT));
                    curr_type->extra.array->dimension =
                        dimension->value.int_value;
                } else {
                    curr_type->kind = TYK_VLA;
                    curr_type->extra.vla_size_expr =
                        curr_declarator->extra.arr_decl->dim_expr;
                }
            }
            tacc_type_list_push(sub_type->derived_array_types, curr_type);
            curr_declarator = curr_declarator->extra.arr_decl->sub_declarator;
            continue;
        }
        /* function declarator */
        sub_type = tacc_type_to_pointer(compiler->target->pointer_ty,
                                        curr_type,
                                        curr_declarator->indirection_level);
        curr_type = tacc_type_new();
        curr_type->kind = TYK_FN;
        tacc_type_list_push(sub_type->derived_func_types, curr_type);
        curr_type->extra.function = tacc_function_type_new();
        curr_type->extra.function->return_type = sub_type;
        tacc_type_adjust_function(compiler,
                                  curr_type->extra.function,
                                  curr_declarator->extra.func_decl);
        curr_declarator = curr_declarator->extra.func_decl->sub_declarator;
    }
}

static void tacc_struct_push_field(struct tacc_compiler *compiler,
                                   struct tacc_struct_type *ty,
                                   struct tacc_field *field) {
    size_t alignment;

    TACC_UNUSED(compiler);

    alignment = tacc_type_alignment_p2(field->type);
    if (alignment < ty->alignment_p2) {
        ty->alignment_p2 = alignment;
    }

    tacc_field_list_push(ty->fields, field);
}
static void tacc_union_push_field(struct tacc_compiler *compiler,
                                  struct tacc_union_type *ty,
                                  struct tacc_field *field) {
    size_t alignment;

    TACC_UNUSED(compiler);

    alignment = tacc_type_alignment_p2(field->type);
    if (alignment < ty->alignment_p2) {
        ty->alignment_p2 = alignment;
    }

    tacc_field_list_push(ty->fields, field);
}

static struct tacc_struct_type *
tacc_eval_struct(struct tacc_compiler *compiler,
                 struct tacc_struct_decl_list *struct_fields) {
    struct tacc_struct_decl_list_entry *entry;
    struct tacc_struct_declarator_list_entry *declarator_entry;
    struct tacc_struct_type *ty;
    struct tacc_field *field;
    struct tacc_type *base_ty;
    struct tacc_type *adjusted_ty;
    size_t i;
    size_t j;
    size_t bit_offset;

    ty = tacc_struct_type_new();
    bit_offset = 0;
    for (i = 0; i < tacc_struct_decl_list_len(struct_fields); i = i + 1) {
        entry = tacc_struct_decl_list_get(struct_fields, i);
        base_ty = tacc_type_from_decl_type(compiler, entry->content->base_type);
        for (j = 0;
             j < tacc_struct_declarator_list_len(entry->content->declarators);
             j = j + 1) {
            declarator_entry =
                tacc_struct_declarator_list_get(entry->content->declarators, i);
            adjusted_ty = tacc_type_adjust_from_declarator(
                compiler, base_ty, declarator_entry->content->underlying);
            field = tacc_field_new();
            field->type = adjusted_ty;
            field->name = tacc_compile_get_name(
                compiler,
                tacc_declarator_name(declarator_entry->content->underlying));
            if (declarator_entry->content->bitfield_size == NULL) {
                bit_offset = tacc_align_up(
                    bit_offset, 3 + tacc_type_alignment_p2(adjusted_ty));
                field->offset = bit_offset >> ((unsigned) 3);
                tacc_struct_push_field(compiler, ty, field);
                bit_offset = bit_offset + (tacc_type_size(adjusted_ty) << 3);
                continue;
            } else {
                tacc_assert(ASSERT_TODO, 0, "evaluate bitfields in structures");
            }
        }
    }
    ty->size = tacc_align_up(bit_offset >> ((unsigned) 3), ty->alignment_p2);

    return ty;
}

static struct tacc_union_type *
tacc_eval_union(struct tacc_compiler *compiler,
                struct tacc_struct_decl_list *struct_fields) {
    struct tacc_struct_decl_list_entry *entry;
    struct tacc_struct_declarator_list_entry *declarator_entry;
    struct tacc_union_type *ty;
    struct tacc_field *field;
    struct tacc_type *base_ty;
    struct tacc_type *adjusted_ty;
    size_t i;
    size_t j;

    ty = tacc_union_type_new();
    for (i = 0; i < tacc_struct_decl_list_len(struct_fields); i = i + 1) {
        entry = tacc_struct_decl_list_get(struct_fields, i);
        base_ty = tacc_type_from_decl_type(compiler, entry->content->base_type);
        for (j = 0;
             j < tacc_struct_declarator_list_len(entry->content->declarators);
             j = j + 1) {
            declarator_entry =
                tacc_struct_declarator_list_get(entry->content->declarators, i);
            adjusted_ty = tacc_type_adjust_from_declarator(
                compiler, base_ty, declarator_entry->content->underlying);
            field = tacc_field_new();
            field->type = adjusted_ty;
            field->name = tacc_compile_get_name(
                compiler,
                tacc_declarator_name(declarator_entry->content->underlying));
            field->offset = 0;
            if (declarator_entry->content->bitfield_size == NULL) {
                tacc_union_push_field(compiler, ty, field);
                continue;
            } else {
                tacc_assert(ASSERT_TODO, 0, "evaluate bitfields in unions");
            }
        }
    }

    return ty;
}

static struct tacc_type *tacc_compiler_get_named_type(
    struct tacc_compiler *compiler, uint32_t name_ref) {
    struct tacc_type_list_entry *entry;
    size_t i;

    /* TODO: just use a hashmap with an uint32_t key */
    for (i = 0; i < tacc_type_list_len(compiler->named_types); i = i + 1) {
        entry = tacc_type_list_get(compiler->named_types, i);
        if (entry->content->name_ref == name_ref) {
            return entry->content;
        }
    }
    return NULL;
}

static struct tacc_type *
tacc_compiler_get_named_type_or_forwdecl(struct tacc_compiler *compiler,
                                         uint32_t name_ref,
                                         enum tacc_type_kind kind) {
    struct tacc_type *ty;

    ty = tacc_compiler_get_named_type(compiler, name_ref);
    if (ty != NULL) {
        tacc_assert(
            ASSERT_DIAG,
            ty->kind == kind,
            "incompatible redeclaration of tag %s",
            tacc_dynstring_as_str(tacc_compile_get_name(compiler, name_ref)));
        return ty;
    }

    ty = tacc_type_new();
    ty->name_ref = name_ref;
    ty->kind = kind;
    tacc_type_list_push(compiler->named_types, ty);
    return ty;
}

static void tacc_compiler_add_new_named_type(struct tacc_compiler *compiler,
                                             struct tacc_type *ty) {
    tacc_assert(ASSERT_DIAG,
                tacc_compiler_get_named_type(compiler, ty->name_ref) == NULL,
                "type %s redeclared",
                tacc_compile_get_name(compiler, ty->name_ref));
    tacc_type_list_push(compiler->named_types, ty);
}

struct tacc_type *tacc_type_from_decl_type(struct tacc_compiler *compiler,
                                           struct tacc_decl_type *type) {
    enum tacc_type_kind base_type;
    uint32_t typespec;
    struct tacc_type *ty;
    struct tacc_type_list_entry *entry;

    typespec = type->spec_qual_flags & ~((uint32_t) TYPEQUAL_CONST |
                                         TYPEQUAL_RESTRICT | TYPEQUAL_VOLATILE);

    switch (typespec) {
    case TYPESPEC_BOOL:
        base_type = TYK_BOOL;
        break;
    case TYPESPEC_CHAR:
        if (compiler->target->signed_char) {
            base_type = TYK_SCHAR;
        } else {
            base_type = TYK_UCHAR;
        }
        break;
    case /*TYPESPEC_UNSIGNED | TYPESPEC_CHAR*/ 0x9:
        base_type = TYK_UCHAR;
        break;
    case /* TYPESPEC_SIGNED | TYPESPEC_CHAR */ 0xa:
        base_type = TYK_SCHAR;
        break;
    case TYPESPEC_SHORT:
    case /* TYPESPEC_SHORT | TYPESPEC_SIGNED */ 0x11:
        base_type = TYK_SSHORT;
        break;
    case /* TYPESPEC_SHORT | TYPESPEC_UNSIGNED */ 0x12:
        base_type = TYK_USHORT;
        break;
    case 0:
    case TYPESPEC_INT:
    case TYPESPEC_SIGNED:
    case /* TYPESPEC_INT | TYPESPEC_SIGNED */ 0x21:
        base_type = TYK_SINT;
        break;
    case TYPESPEC_UNSIGNED:
    case /* TYPESPEC_INT | TYPESPEC_UNSIGNED */ 0x22:
        base_type = TYK_UINT;
        break;
    case TYPESPEC_LONG:
    case /* TYPESPEC_LONG | TYPESPEC_SIGNED */ 0x41:
    case /* TYPESPEC_LONG | TYPESPEC_INT */ 0x60:
    case /* TYPESPEC_LONG | TYPESPEC_INT | TYPESPEC_SIGNED */ 0x61:
        base_type = TYK_SLONG;
        break;
    case /* TYPESPEC_LONG | TYPESPEC_UNSIGNED */ 0x42:
    case /* TYPESPEC_LONG | TYPESPEC_UNSIGNED | TYPESPEC_INT */ 0x62:
        base_type = TYK_ULONG;
        break;
    case TYPESPEC_LONG_2:
    case /* TYPESPEC_LONG_2 | TYPESPEC_SIGNED */ 0x81:
    case /* TYPESPEC_LONG_2 | TYPESPEC_INT */ 0xa0:
    case /* TYPESPEC_LONG_2 | TYPESPEC_INT | TYPESPEC_SIGNED */ 0xa1:
        base_type = TYK_SLONGLONG;
        break;
    case /* TYPESPEC_LONG_2 | TYPESPEC_UNSIGNED */ 0x82:
    case /* TYPESPEC_LONG_2 | TYPESPEC_INT | TYPESPEC_UNSIGNED */ 0xa2:
        base_type = TYK_ULONGLONG;
        break;
    case TYPESPEC_DOUBLE:
        base_type = TYK_DOUBLE;
        break;
    case /* TYPESPEC_LONG | TYPESPEC_DOUBLE */ 0x240:
        base_type = TYK_LONGDOUBLE;
        break;
    case TYPESPEC_FLOAT:
        base_type = TYK_FLOAT;
        break;
    case TYPESPEC_VOID:
        base_type = TYK_VOID;
        break;
    case TYPESPEC_ENUM:
        base_type = TYK_ENUM;
        if (type->extra.enumerators == NULL) {
            tacc_assert(ASSERT_DIAG,
                        type->name_ref != 0,
                        "anonymous unspecified enumeration");
            ty = tacc_compiler_get_named_type(compiler, type->name_ref);
            tacc_assert(ASSERT_DIAG,
                        ty != NULL,
                        "forward declaration of enum %s",
                        tacc_dynstring_as_str(
                            tacc_compile_get_name(compiler, type->name_ref)));
        } else {
            ty = tacc_type_new();
            ty->kind = TYK_ENUM;
            ty->extra.enumeration = tacc_enumeration_type_new();
            ty->extra.enumeration->underlying_type =
                tacc_eval_enumerators(compiler, type->extra.enumerators);
            if (type->name_ref != 0) {
                ty->name_ref = type->name_ref;
                tacc_compiler_add_new_named_type(compiler, ty);
            } else {
                tacc_type_list_push(compiler->anonymous_types, ty);
            }
        }
        break;
    case TYPESPEC_STRUCT:
    case TYPESPEC_UNION:
        if (typespec == TYPESPEC_STRUCT) {
            base_type = TYK_STRUCT;
        } else {
            base_type = TYK_UNION;
        }
        if (type->extra.struct_fields == NULL) {
            tacc_assert(ASSERT_DIAG,
                        type->name_ref != 0,
                        "anonymous unspecified struct/union");
            ty = tacc_compiler_get_named_type_or_forwdecl(
                compiler, type->name_ref, base_type);
        } else {
            ty = tacc_type_new();
            ty->kind = base_type;
            if (typespec == TYPESPEC_STRUCT) {
                ty->extra.structure =
                    tacc_eval_struct(compiler, type->extra.struct_fields);
            } else {
                ty->extra.onion =
                    tacc_eval_union(compiler, type->extra.struct_fields);
            }
            if (type->name_ref != 0) {
                ty->name_ref = type->name_ref;
                /*
                 * May result in merge of past forward-declaration,
                 * returning the tacc_type from that forward-declaration.
                 */
                tacc_compiler_add_new_named_type(compiler, ty);
            } else {
                tacc_type_list_push(compiler->anonymous_types, ty);
            }
        }
        break;
    case TYPESPEC_TYPEDEF:
        tacc_assert(ASSERT_TODO, 0, "construct type from typedef");
        return NULL;
    default:
        tacc_assert(ASSERT_ICE, 0, "type unsupported as of now");
        return NULL;
    }

    entry = tacc_type_list_get(compiler->basic_types, base_type);
    ty = entry->content;

    return ty;
}

static void tacc_compile_function_def(struct tacc_compiler *compiler,
                                      struct tacc_decl *function_def) {
    struct tacc_string *func_name;
    struct tacc_type *function_type;
    struct tacc_cg_state *state;
    struct tacc_global_object *global_object;

    function_type = tacc_type_adjust_from_declarator(
        compiler,
        tacc_type_from_decl_type(compiler, function_def->base_type),
        function_def->extra.func_def->func_declaration);

    global_object = tacc_global_object_new();
    global_object->is_enumerator = 0;
    global_object->name_ref =
        tacc_declarator_name(function_def->extra.func_def->func_declaration);
    global_object->extra.obj_type = function_type;
    tacc_compile_add_global_object(compiler, global_object);

    tacc_assert(ASSERT_TODO,
                function_def->extra.func_def->old_style_param_list == NULL,
                "old-style function parameter types");

    state = tacc_cg_state_new(compiler);
    tacc_cg_compile_function(
        state, function_def->extra.func_def, function_type->extra.function);

    tacc_compile_output_directive(compiler, "section .text, \"ax\", @progbits");
    func_name = tacc_compile_get_name(
        compiler,
        tacc_declarator_name(function_def->extra.func_def->func_declaration));
    tacc_compile_output_directive(
        compiler, "globl %s", tacc_dynstring_as_str(func_name));
    tacc_compile_output(compiler, "\n%s:", tacc_dynstring_as_str(func_name));
    tacc_cg_finalize(state);
    tacc_compile_output(
        compiler, "%s", tacc_dynstring_as_str(state->prelude_buffer));
    tacc_compile_output(
        compiler, "%s", tacc_dynstring_as_str(state->code_buffer));

    tacc_cg_state_free(state);
    state = NULL;
}

void tacc_compile_prelude(struct tacc_compiler *compiler) {
    tacc_target_cg_prelude(compiler);
}

struct tacc_string *tacc_compile_get_name(struct tacc_compiler *compiler,
                                          uint32_t name_ref) {
    struct tacc_string_list_entry *entry;

    entry = tacc_string_list_get(compiler->names, name_ref);

    return entry->content;
}

void tacc_compile_top_decl(struct tacc_compiler *compiler,
                           struct tacc_decl *decl) {
    struct tacc_type *type;
    struct tacc_global_object *global_object;
    size_t i;
    struct tacc_init_declarator_list_entry *entry;
    struct tacc_init_declarator *declarator;
    uint32_t data_name;

    if (decl->kind == DECL_FUNCTION_DEF) {
        tacc_compile_function_def(compiler, decl);
        return;
    }
    tacc_assert(ASSERT_TODO,
                decl->storage_class == STORAGE_UNSPECIFIED,
                "different storage classes");
    type = tacc_type_from_decl_type(compiler, decl->base_type);
    for (i = 0; i < tacc_init_declarator_list_len(decl->extra.declarators);
         i = i + 1) {
        entry = tacc_init_declarator_list_get(decl->extra.declarators, i);
        data_name = tacc_declarator_name(entry->content->declarator);
        declarator = entry->content;
        tacc_assert(ASSERT_TODO,
                    declarator->declarator->kind == DECLARATOR_PLAIN,
                    "non-plain declarator");

        global_object = tacc_global_object_new();
        global_object->is_enumerator = 0;
        global_object->name_ref = data_name;
        global_object->extra.obj_type = type;
        tacc_compile_add_global_object(compiler, global_object);

        tacc_compile_data(compiler,
                          type,
                          tacc_compile_get_name(compiler, data_name),
                          declarator->initializer);
    }
    tacc_compile_output(compiler, "\n");
}

struct tacc_global_object *
tacc_compile_resolve_global(struct tacc_compiler *compiler, uint32_t name_ref) {
    struct tacc_global_object_map_entry *entry;

    entry = tacc_global_object_map_get(compiler->global_objects, name_ref);
    if (entry != NULL) {
        return entry->content;
    }
    return NULL;
}

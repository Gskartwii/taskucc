#include "compile.h"
#include "codegen.h"
#include "decl.h"
#include "dynstring.h"
#include "expr.h"
#include "global_object.h"
#include "initializer.h"
#include "machine.h"
#include "predef_func.h"
#include "soft_float.h"
#include "string_list.h"
#include "target/codegen.h"
#include "type.h"
#include "util.h"
#include <stdarg.h>

static void tacc_ident_free(uint32_t ident) { TACC_UNUSED(ident); }

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

static void tacc_named_type_free(struct tacc_named_type *ty) { tacc_free(ty); }

MK_DYNHASH_OVER_U32(tacc_type_map,
                    name,
                    tacc_type_map_entry,
                    struct tacc_named_type *,
                    tacc_type_map_new,
                    tacc_type_map_init,
                    tacc_type_map_get,
                    tacc_type_map_insert,
                    tacc_type_map_fill_count,
                    tacc_named_type_free,
                    tacc_type_map_free)

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

struct tacc_val *tacc_compile_const_eval(struct tacc_compiler *compiler,
                                         struct tacc_expr *expr) {
    struct tacc_eval_context ctx;

    ctx.basic_types = compiler->basic_types;
    ctx.global_objects = compiler->global_objects;
    ctx.target = compiler->target;

    return tacc_expr_const_eval(expr, &ctx);
}

static void tacc_compile_serialize_int(char *buf,
                                       struct tacc_type *as_type,
                                       struct tacc_val *val) {
    char *pos;
    size_t length;
    size_t i;
    struct tacc_u64 aux;

    length = tacc_type_size(as_type);
    for (i = 0; i < length; i = i + 1) {
        tacc_u64_rsh_n(&aux, val->value.int_value, (int) (8 * i));
        pos = buf + i;
        *pos = (char) (aux.low & 0xFF);
    }
}

static void tacc_compile_data(struct tacc_compiler *compiler,
                              struct tacc_type *for_type,
                              struct tacc_string *name,
                              struct tacc_initializer *initializer,
                              enum tacc_storage_class storage) {
    struct tacc_eval_context ctx;
    struct tacc_init_command_list *cmd_list;
    struct tacc_init_command_list_entry *entry;
    struct tacc_val *val;
    size_t obj_size;
    size_t i;
    char *obj_buf;
    char *obj_location;

    ctx.basic_types = compiler->basic_types;
    ctx.global_objects = compiler->global_objects;
    ctx.target = compiler->target;

    /* may update for_type to turn it from incomplete to complete array */
    cmd_list = tacc_init_evaluate_initializer(initializer, for_type, &ctx);
    obj_size = tacc_type_size(for_type);
    obj_buf = tacc_malloc(obj_size);
    memset(obj_buf, 0, obj_size);

    /*
     * TODO: we take sizeof() at eval time; this means types defined therein
     * would be defined at eval time... ugh.
     */
    for (i = 0; i < tacc_init_command_list_len(cmd_list); i = i + 1) {
        entry = tacc_init_command_list_get(cmd_list, i);
        obj_location = obj_buf + entry->content->offset;
        tacc_assert(ASSERT_TODO,
                    tacc_type_is_integral(entry->content->as_type),
                    "non-integral static initializers");
        val = tacc_expr_const_eval(entry->content->stored_expr, &ctx);
        tacc_assert(
            ASSERT_DIAG, val != NULL, "non-constant static initializer");
        tacc_compile_serialize_int(obj_location, entry->content->as_type, val);
        tacc_val_free(val);
        val = NULL;
    }
    tacc_init_command_list_free(cmd_list);
    tacc_free(cmd_list);
    cmd_list = NULL;

    tacc_compile_output_directive(compiler, "data");
    if (storage == STORAGE_UNSPECIFIED || storage == STORAGE_EXTERN) {
        tacc_compile_output_directive(
            compiler, "global %s", tacc_dynstring_as_str(name));
    }
    tacc_compile_output_directive(
        compiler, "type %s, %%object", tacc_dynstring_as_str(name));
    tacc_compile_output(compiler, "\n%s:", tacc_dynstring_as_str(name));
    tacc_compile_output_directive(
        compiler, "p2align %d", tacc_type_alignment_p2(for_type));
    for (i = 0; i < obj_size; i = i + 1) {
        obj_location = obj_buf + i;
        tacc_compile_output_directive(
            compiler, "byte 0x%x", (uint32_t) ((*obj_location) & 0xFF));
    }
    tacc_free(obj_buf);
    obj_buf = NULL;
}

static struct tacc_type *tacc_eval_enumerators(
    struct tacc_compiler *compiler, struct tacc_enumerator_list *enumerators) {
    struct tacc_u64 counter;
    size_t i;
    struct tacc_enumerator_list_entry *entry;
    struct tacc_val *val;
    struct tacc_global_object *obj;
    tacc_bool use_negative;

    use_negative = 0;
    tacc_u64_zero(&counter);

    for (i = 0; i < tacc_enumerator_list_len(enumerators); i = i + 1) {
        entry = tacc_enumerator_list_get(enumerators, i);
        obj = tacc_global_object_new();
        obj->is_enumerator = 1;
        obj->name_ref = entry->content->name_ref;

        if (entry->content->value != NULL) {
            val = tacc_compile_const_eval(compiler, entry->content->value);
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
            obj->extra.enumerator_value = tacc_u64_new();
            tacc_u64_copy(obj->extra.enumerator_value, val->value.int_value);
            tacc_u64_copy(&counter, val->value.int_value);
            tacc_val_free(val);
        } else {
            tacc_assert(ASSERT_DIAG,
                        counter.low != 0xFFFFFFFF || counter.high != 0xFFFFFFFF,
                        "enumerator overflow when implicitly incrementing");
            obj->extra.enumerator_value = tacc_u64_new();
            tacc_u64_copy(obj->extra.enumerator_value, &counter);
        }
        tacc_u64_add_u32(&counter, &counter, 1);
        tacc_compile_add_global_object(compiler, obj);
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
                dimension = tacc_compile_const_eval(
                    compiler, curr_declarator->extra.arr_decl->dim_expr);
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
                        tacc_u64_clone(dimension->value.int_value);
                    tacc_val_free(dimension);
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
                tacc_struct_declarator_list_get(entry->content->declarators, j);
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
                tacc_struct_declarator_list_get(entry->content->declarators, j);
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
tacc_compiler_get_typedef(struct tacc_compiler *compiler, uint32_t name_ref) {
    struct tacc_type_map_entry *entry;

    entry = tacc_type_map_get(compiler->typedefs, name_ref);
    if (entry != NULL) {
        return entry->content->type;
    }

    return NULL;
}

static struct tacc_type *
tacc_compiler_get_named_type_or_forwdecl(struct tacc_compiler *compiler,
                                         uint32_t name_ref,
                                         enum tacc_type_kind kind,
                                         tacc_bool *made_new) {
    struct tacc_type *ty;

    ty = tacc_compiler_get_named_type(compiler, name_ref);
    if (ty != NULL) {
        tacc_assert(
            ASSERT_DIAG,
            ty->kind == kind,
            "incompatible redeclaration of tag %s",
            tacc_dynstring_as_str(tacc_compile_get_name(compiler, name_ref)));
        *made_new = 0;
        return ty;
    }

    ty = tacc_type_new();
    ty->name_ref = name_ref;
    ty->kind = kind;
    tacc_type_list_push(compiler->named_types, ty);
    *made_new = 1;
    return ty;
}

static struct tacc_type *tacc_compiler_add_new_named_type_or_bind(
    struct tacc_compiler *compiler, struct tacc_type *ty) {
    struct tacc_type *existing;
    char *name;

    existing = tacc_compiler_get_named_type(compiler, ty->name_ref);
    if (existing == NULL) {
        tacc_type_list_push(compiler->named_types, ty);
        return ty;
    }
    name = tacc_dynstring_as_str(tacc_compile_get_name(compiler, ty->name_ref));
    tacc_assert(ASSERT_DIAG,
                existing->kind == ty->kind,
                "type tag for %s is incompatible with previous tagged type",
                name);
    if (ty->kind == TYK_STRUCT) {
        tacc_assert(ASSERT_DIAG,
                    existing->extra.structure == NULL,
                    "struct %s redefined",
                    name);
        existing->extra.structure = ty->extra.structure;
        ty->extra.structure = NULL;
    } else {
        tacc_assert(ASSERT_DIAG,
                    existing->extra.onion == NULL,
                    "union %s redefined",
                    name);
        existing->extra.onion = ty->extra.onion;
    }
    tacc_type_free(ty);

    return existing;
}

struct tacc_type *tacc_type_from_decl_type(struct tacc_compiler *compiler,
                                           struct tacc_decl_type *type) {
    enum tacc_type_kind base_type;
    uint32_t typespec;
    struct tacc_type *ty;
    struct tacc_type_list_entry *entry;
    tacc_bool is_forward_declare;

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
    case /* TYPESPEC_UNSIGNED | TYPESPEC_CHAR */ 0x9:
        base_type = TYK_UCHAR;
        break;
    case /* TYPESPEC_SIGNED | TYPESPEC_CHAR */ 0xa:
        base_type = TYK_SCHAR;
        break;
    case TYPESPEC_SHORT:
    case /* TYPESPEC_SHORT | TYPESPEC_INT */ 0x30:
    case /* TYPESPEC_SHORT | TYPESPEC_SIGNED */ 0x12:
    case /* TYPESPEC_SHORT | TYPESPEC_INT | TYPESPEC_SIGNED */ 0x32:
        base_type = TYK_SSHORT;
        break;
    case /* TYPESPEC_SHORT | TYPESPEC_UNSIGNED | TYPESPEC_INT */ 0x31:
    case /* TYPESPEC_SHORT | TYPESPEC_UNSIGNED */ 0x11:
        base_type = TYK_USHORT;
        break;
    case 0:
    case TYPESPEC_INT:
    case TYPESPEC_SIGNED:
    case /* TYPESPEC_INT | TYPESPEC_SIGNED */ 0x22:
        base_type = TYK_SINT;
        break;
    case TYPESPEC_UNSIGNED:
    case /* TYPESPEC_INT | TYPESPEC_UNSIGNED */ 0x21:
        base_type = TYK_UINT;
        break;
    case TYPESPEC_LONG:
    case /* TYPESPEC_LONG | TYPESPEC_SIGNED */ 0x42:
    case /* TYPESPEC_LONG | TYPESPEC_INT */ 0x60:
    case /* TYPESPEC_LONG | TYPESPEC_INT | TYPESPEC_SIGNED */ 0x62:
        base_type = TYK_SLONG;
        break;
    case /* TYPESPEC_LONG | TYPESPEC_UNSIGNED */ 0x41:
    case /* TYPESPEC_LONG | TYPESPEC_UNSIGNED | TYPESPEC_INT */ 0x61:
        base_type = TYK_ULONG;
        break;
    case TYPESPEC_LONG_2:
    case /* TYPESPEC_LONG_2 | TYPESPEC_SIGNED */ 0x82:
    case /* TYPESPEC_LONG_2 | TYPESPEC_INT */ 0xa0:
    case /* TYPESPEC_LONG_2 | TYPESPEC_INT | TYPESPEC_SIGNED */ 0xa2:
        base_type = TYK_SLONGLONG;
        break;
    case /* TYPESPEC_LONG_2 | TYPESPEC_UNSIGNED */ 0x81:
    case /* TYPESPEC_LONG_2 | TYPESPEC_INT | TYPESPEC_UNSIGNED */ 0xa1:
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
                ty = tacc_compiler_add_new_named_type_or_bind(compiler, ty);
            } else {
                tacc_type_list_push(compiler->anonymous_types, ty);
            }
        }
        return ty->extra.enumeration->underlying_type;
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
                compiler, type->name_ref, base_type, &is_forward_declare);
            if (is_forward_declare) {
                if (base_type == TYK_STRUCT) {
                    ty->extra.structure = NULL;
                } else {
                    ty->extra.onion = NULL;
                }
            }
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
                ty = tacc_compiler_add_new_named_type_or_bind(compiler, ty);
            } else {
                tacc_type_list_push(compiler->anonymous_types, ty);
            }
        }
        return ty;
    case TYPESPEC_TYPEDEF:
        ty = tacc_compiler_get_typedef(compiler, type->name_ref);
        tacc_assert(ASSERT_DIAG,
                    ty != NULL,
                    "non-existent typedef: %s",
                    tacc_dynstring_as_str(
                        tacc_compile_get_name(compiler, type->name_ref)));
        return ty;
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

    if (function_def->storage_class == STORAGE_EXTERN ||
        function_def->storage_class == STORAGE_UNSPECIFIED) {
        tacc_compile_output_directive(
            compiler, "globl %s", tacc_dynstring_as_str(func_name));
    } else {
        tacc_assert(ASSERT_DIAG,
                    function_def->storage_class == STORAGE_STATIC,
                    "unexpected storage class on function definition");
    }

    tacc_compile_output(compiler, "\n%s:", tacc_dynstring_as_str(func_name));
    tacc_cg_finalize(state);
    tacc_compile_output(
        compiler, "%s", tacc_dynstring_as_str(state->prelude_buffer));
    tacc_compile_output(
        compiler, "%s", tacc_dynstring_as_str(state->code_buffer));

    tacc_cg_state_free(state);
    state = NULL;
}

static void tacc_compile_declare_predef(struct tacc_compiler *compiler,
                                        enum tacc_predef_func_id id,
                                        struct tacc_type *arg_ty,
                                        struct tacc_type *ret_ty) {
    struct tacc_global_object *object;
    struct tacc_type *func_ty;

    object = tacc_global_object_new();
    object->is_enumerator = 0;
    object->name_ref = (uint32_t) -id;

    func_ty = tacc_type_new();
    func_ty->kind = TYK_FN;
    func_ty->extra.function = tacc_function_type_new();
    func_ty->extra.function->return_type = ret_ty;
    func_ty->extra.function->param_types = tacc_type_list_new();
    tacc_type_list_push(func_ty->extra.function->param_types, arg_ty);
    object->extra.obj_type = func_ty;

    tacc_global_object_map_insert(compiler->global_objects, object);
    tacc_type_list_push(ret_ty->derived_func_types, func_ty);
}

static void tacc_compile_declare_predef_2(struct tacc_compiler *compiler,
                                          enum tacc_predef_func_id id,
                                          struct tacc_type *arg_ty_1,
                                          struct tacc_type *arg_ty_2,
                                          struct tacc_type *ret_ty) {
    struct tacc_global_object *object;
    struct tacc_type *func_ty;

    object = tacc_global_object_new();
    object->is_enumerator = 0;
    object->name_ref = (uint32_t) -id;

    func_ty = tacc_type_new();
    func_ty->kind = TYK_FN;
    func_ty->extra.function = tacc_function_type_new();
    func_ty->extra.function->return_type = ret_ty;
    func_ty->extra.function->param_types = tacc_type_list_new();
    tacc_type_list_push(func_ty->extra.function->param_types, arg_ty_1);
    tacc_type_list_push(func_ty->extra.function->param_types, arg_ty_2);
    object->extra.obj_type = func_ty;

    tacc_global_object_map_insert(compiler->global_objects, object);
    tacc_type_list_push(ret_ty->derived_func_types, func_ty);
}

static void tacc_compile_add_predef_name(struct tacc_compiler *compiler,
                                         char *name) {
    struct tacc_string *name_string;

    name_string = tacc_dynstring_new();
    tacc_dynstring_concat(name_string, name);

    tacc_string_list_push(compiler->predef_names, name_string);
}

static void tacc_compile_init_predef(struct tacc_compiler *compiler) {
    struct tacc_type *longdouble;
    struct tacc_type *sfloat;
    struct tacc_type *dfloat;
    struct tacc_type *llongint;
    struct tacc_type *llonguint;
    struct tacc_type *sint;
    struct tacc_type_list *basic_types;

    basic_types = compiler->basic_types;
    longdouble = tacc_get_basic_type(basic_types, TYK_LONGDOUBLE);
    sfloat = tacc_get_basic_type(basic_types, TYK_FLOAT);
    dfloat = tacc_get_basic_type(basic_types, TYK_DOUBLE);
    llongint = tacc_get_basic_type(basic_types, TYK_SLONGLONG);
    llonguint = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
    sint = tacc_get_basic_type(basic_types, TYK_SINT);

    tacc_compile_add_predef_name(compiler, "__taccrt_fixtfdi");
    tacc_compile_add_predef_name(compiler, "__taccrt_fixunstfdi");
    tacc_compile_add_predef_name(compiler, "__taccrt_floatditf");
    tacc_compile_add_predef_name(compiler, "__taccrt_floatunsditf");
    tacc_compile_add_predef_name(compiler, "__taccrt_fixtfsi");
    tacc_compile_add_predef_name(compiler, "__taccrt_trunctfsf2");
    tacc_compile_add_predef_name(compiler, "__taccrt_trunctfdf2");
    tacc_compile_add_predef_name(compiler, "__taccrt_extendsftf2");
    tacc_compile_add_predef_name(compiler, "__taccrt_extenddftf2");
    tacc_compile_add_predef_name(compiler, "__taccrt_muldi3");
    tacc_compile_add_predef_name(compiler, "__taccrt_divdi3");
    tacc_compile_add_predef_name(compiler, "__taccrt_udivdi3");
    tacc_compile_add_predef_name(compiler, "__taccrt_moddi3");
    tacc_compile_add_predef_name(compiler, "__taccrt_umoddi3");
    tacc_compile_add_predef_name(compiler, "__taccrt_addtf3");
    tacc_compile_add_predef_name(compiler, "__taccrt_subtf3");
    tacc_compile_add_predef_name(compiler, "__taccrt_multf3");
    tacc_compile_add_predef_name(compiler, "__taccrt_divtf3");
    tacc_compile_add_predef_name(compiler, "__taccrt_eqtf2");
    tacc_compile_add_predef_name(compiler, "__taccrt_netf2");
    tacc_compile_add_predef_name(compiler, "__taccrt_letf2");
    tacc_compile_add_predef_name(compiler, "__taccrt_lttf2");

    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_FIXTFDI, longdouble, llongint);
    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_FIXUNSTFDI, longdouble, llonguint);
    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_FLOATDITF, llongint, longdouble);
    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_FLOATUNSDITF, llonguint, longdouble);
    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_FIXTFSI, longdouble, sint);
    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_TRUNCTFSF2, longdouble, sfloat);
    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_TRUNCTFDF2, longdouble, dfloat);
    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_EXTENDSFTF2, sfloat, longdouble);
    tacc_compile_declare_predef(
        compiler, PREDEF__TACCRT_EXTENDDFTF2, dfloat, longdouble);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_MULDI3, llongint, llongint, llongint);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_DIVDI3, llongint, llongint, llongint);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_UDIVDI3, llonguint, llonguint, llonguint);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_MODDI3, llongint, llongint, llongint);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_UMODDI3, llonguint, llonguint, llonguint);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_ADDTF3, longdouble, longdouble, longdouble);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_SUBTF3, longdouble, longdouble, longdouble);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_MULTF3, longdouble, longdouble, longdouble);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_DIVTF3, longdouble, longdouble, longdouble);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_EQTF2, longdouble, longdouble, sint);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_NETF2, longdouble, longdouble, sint);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_LETF2, longdouble, longdouble, sint);
    tacc_compile_declare_predef_2(
        compiler, PREDEF__TACCRT_LTTF2, longdouble, longdouble, sint);
}

void tacc_compile_prelude(struct tacc_compiler *compiler) {
    tacc_compile_init_predef(compiler);
    tacc_target_cg_prelude(compiler);
}

struct tacc_string *tacc_compile_get_name(struct tacc_compiler *compiler,
                                          uint32_t name_ref) {
    struct tacc_string_list_entry *entry;
    uint32_t adjusted_id;

    if (((name_ref >> 31) & 1) != 0) {
        adjusted_id = -name_ref;
        entry = tacc_string_list_get(compiler->predef_names, adjusted_id);
    } else {
        entry = tacc_string_list_get(compiler->names, name_ref);
    }

    return entry->content;
}

struct tacc_named_type *tacc_named_type_new(void) {
    struct tacc_named_type *named_type;

    named_type = tacc_malloc(sizeof(struct tacc_named_type));
    named_type->type = NULL;
    named_type->name = 0;

    return named_type;
}

static void tacc_compile_typedef(struct tacc_compiler *compiler,
                                 struct tacc_decl *decl) {
    struct tacc_type *type;
    struct tacc_type *derived_type;
    struct tacc_type *preexisting_type;
    struct tacc_init_declarator_list_entry *entry;
    struct tacc_init_declarator *declarator;
    struct tacc_named_type *named_type;
    uint32_t ty_name;
    size_t i;

    type = tacc_type_from_decl_type(compiler, decl->base_type);

    for (i = 0; i < tacc_init_declarator_list_len(decl->extra.declarators);
         i = i + 1) {
        entry = tacc_init_declarator_list_get(decl->extra.declarators, i);
        ty_name = tacc_declarator_name(entry->content->declarator);
        declarator = entry->content;
        tacc_assert(ASSERT_DIAG,
                    declarator->initializer == NULL,
                    "initializer on a typedef");
        derived_type = tacc_type_adjust_from_declarator(
            compiler, type, declarator->declarator);

        preexisting_type = tacc_compiler_get_typedef(compiler, ty_name);
        tacc_assert(
            ASSERT_DIAG,
            preexisting_type == NULL,
            "redefinition of typedef %s",
            tacc_dynstring_as_str(tacc_compile_get_name(compiler, ty_name)));
        named_type = tacc_named_type_new();
        named_type->name = ty_name;
        named_type->type = derived_type;

        tacc_type_map_insert(compiler->typedefs, named_type);
    }
}

void tacc_compile_top_decl(struct tacc_compiler *compiler,
                           struct tacc_decl *decl) {
    struct tacc_type *type;
    struct tacc_type *derived_type;
    struct tacc_global_object *global_object;
    size_t i;
    struct tacc_init_declarator_list_entry *entry;
    struct tacc_init_declarator *declarator;
    uint32_t data_name;

    if (decl->kind == DECL_FUNCTION_DEF) {
        tacc_compile_function_def(compiler, decl);
        return;
    }
    if (decl->storage_class == STORAGE_TYPEDEF) {
        tacc_compile_typedef(compiler, decl);
        return;
    }
    type = tacc_type_from_decl_type(compiler, decl->base_type);
    for (i = 0; i < tacc_init_declarator_list_len(decl->extra.declarators);
         i = i + 1) {
        entry = tacc_init_declarator_list_get(decl->extra.declarators, i);
        data_name = tacc_declarator_name(entry->content->declarator);
        declarator = entry->content;
        derived_type = tacc_type_adjust_from_declarator(
            compiler, type, declarator->declarator);

        global_object = tacc_compile_resolve_global(compiler, data_name);
        if (global_object == NULL) {
            global_object = tacc_global_object_new();
            global_object->is_enumerator = 0;
            global_object->name_ref = data_name;
            global_object->extra.obj_type = derived_type;
            tacc_compile_add_global_object(compiler, global_object);
        } else {
            tacc_assert(ASSERT_DIAG,
                        !global_object->is_enumerator,
                        "enumerator name reused for external definition");
            tacc_assert(ASSERT_DIAG,
                        tacc_type_is_compatible(global_object->extra.obj_type,
                                                derived_type),
                        "global object redefinition has incompatible type");
            if (declarator->initializer != NULL) {
                tacc_assert(ASSERT_DIAG,
                            global_object->is_tentative,
                            "two initializers for global object");
            }
            /* TODO: check that storage classes are compatible */
        }

        if (derived_type->kind == TYK_FN) {
            tacc_assert(ASSERT_DIAG,
                        declarator->initializer == NULL,
                        "initializer on a function declarator");
            if (decl->storage_class == STORAGE_STATIC) {
                continue;
            }
            tacc_assert(ASSERT_DIAG,
                        decl->storage_class == STORAGE_EXTERN ||
                            decl->storage_class == STORAGE_UNSPECIFIED,
                        "bad storage class on function declaration");
            tacc_compile_output_directive(
                compiler,
                "globl %s",
                tacc_dynstring_as_str(
                    tacc_compile_get_name(compiler, data_name)));
            continue;
        }

        if (declarator->initializer == NULL) {
            if (decl->storage_class != STORAGE_EXTERN) {
                global_object->is_tentative = 1;
                tacc_global_object_list_push(compiler->maybe_tentative_globals,
                                             global_object);
            }
        } else {
            tacc_compile_data(compiler,
                              derived_type,
                              tacc_compile_get_name(compiler, data_name),
                              declarator->initializer,
                              decl->storage_class);
        }
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

int tacc_compile_emit_local_float(struct tacc_compiler *compiler,
                                  struct tacc_f128 *float_value) {
    int index;
    struct tacc_u64 u64_part;
    struct tacc_u64 u64_part_2;
    uint16_t f80_high16;

    index = compiler->local_obj_index;
    compiler->local_obj_index = index + 1;

    tacc_compile_output_directive(compiler, "data");
    tacc_compile_output_directive(
        compiler,
        "p2align %d",
        (int) (compiler->target->ldouble_ty->alignment_p2));
    tacc_compile_output(compiler, "\n.Lfloat_%d:", index);
    if (compiler->target->ldouble_ty->bit_width == 80) {
        tacc_f128_serialize_round_f80(&u64_part, &f80_high16, float_value);
        /* little endian */
        tacc_compile_output_int(compiler, &u64_part, 64);
        tacc_compile_output_directive(
            compiler, "byte %u", (unsigned) (f80_high16 & 0xFF));
        tacc_compile_output_directive(
            compiler, "byte %u", (unsigned) ((f80_high16 >> 8) & 0xFF));
    } else {
        tacc_f128_serialize_f128(&u64_part, &u64_part_2, float_value);
        tacc_compile_output_int(compiler, &u64_part, 64);
        tacc_compile_output_int(compiler, &u64_part_2, 64);
    }

    return index;
}

int tacc_compile_emit_string_lit(struct tacc_compiler *compiler,
                                 struct tacc_string *string) {
    int index;
    size_t i;

    index = compiler->local_obj_index;
    compiler->local_obj_index = index + 1;

    tacc_compile_output_directive(compiler, "data");
    tacc_compile_output(compiler, "\n.Lstring_%d:", index);

    for (i = 0; i < tacc_dynstring_len(string); i = i + 1) {
        tacc_compile_output_directive(
            compiler, "byte 0x%x", (unsigned) tacc_dynstring_at(string, i));
    }
    tacc_compile_output_directive(compiler, "byte 0");

    return index;
}

struct tacc_type *tacc_get_ptrdiff_type(struct tacc_compiler *compiler) {
    if (compiler->target->pointer_ty->bit_width == 32) {
        return tacc_get_basic_type(compiler->basic_types, TYK_SINT);
    } else {
        return tacc_get_basic_type(compiler->basic_types, TYK_SLONG);
    }
}

struct tacc_type *tacc_get_size_type(struct tacc_compiler *compiler) {
    if (compiler->target->pointer_ty->bit_width == 32) {
        return tacc_get_basic_type(compiler->basic_types, TYK_UINT);
    } else {
        return tacc_get_basic_type(compiler->basic_types, TYK_ULONG);
    }
}

struct tacc_type *tacc_get_uintptr_type(struct tacc_compiler *compiler) {
    if (compiler->target->pointer_ty->bit_width == 32) {
        return tacc_get_basic_type(compiler->basic_types, TYK_UINT);
    } else {
        return tacc_get_basic_type(compiler->basic_types, TYK_ULONG);
    }
}

void tacc_compile_coda(struct tacc_compiler *compiler) {
    struct tacc_global_object_list_entry *entry;
    struct tacc_type *ty;
    size_t i;

    for (i = 0;
         i < tacc_global_object_list_len(compiler->maybe_tentative_globals);
         i = i + 1) {
        entry =
            tacc_global_object_list_get(compiler->maybe_tentative_globals, i);
        if (!entry->content->is_tentative) {
            /* defined after being initially tentative */
            continue;
        }
        ty = entry->content->extra.obj_type;
        tacc_compile_output_directive(compiler, "data");
        tacc_compile_output_directive(
            compiler, "p2align %d", tacc_type_alignment_p2(ty));
        tacc_compile_output(compiler,
                            "\n%s:",
                            tacc_dynstring_as_str(tacc_compile_get_name(
                                compiler, entry->content->name_ref)));
        tacc_compile_output_directive(compiler, "zero %d", tacc_type_size(ty));
    }
}

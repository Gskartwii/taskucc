#include "compile.h"
#include "codegen.h"
#include "decl.h"
#include "dynarray.h"
#include "expr.h"
#include "machine.h"
#include "string_list.h"
#include "target/codegen.h"
#include "type.h"
#include <stdarg.h>

MK_DYNARRAY_OVER(tacc_block_scope_list,
                 tacc_block_scope_list_entry,
                 struct tacc_block_scope *,
                 tacc_block_scope_list_new,
                 tacc_block_scope_list_init,
                 tacc_block_scope_list_get,
                 tacc_block_scope_list_push,
                 tacc_block_scope_list_pop,
                 tacc_block_scope_list_len,
                 tacc_block_scope_free,
                 tacc_block_scope_list_free)
MK_DYNHASH_OVER(tacc_compile_ident_map,
                ident->string,
                tacc_compile_ident_map_entry,
                struct tacc_compile_ident *,
                tacc_compile_ident_map_new,
                tacc_compile_ident_map_init,
                tacc_compile_ident_map_get,
                tacc_compile_ident_map_push,
                tacc_compile_ident_map_count,
                tacc_compile_ident_free,
                tacc_compile_ident_map_free)

void tacc_compile_output_directive(struct tacc_compiler *compiler,
                                   char *directive_fmt,
                                   ...) {
    va_list va;

#ifndef __M2__
    (void) compiler;
#endif

    va_start(va, directive_fmt);
    printf("\n\t.");
    vprintf(directive_fmt, va);
    va_end(va);
}

void tacc_compile_output(struct tacc_compiler *compiler, char *fmt, ...) {
    va_list va;

#ifndef __M2__
    (void) compiler;
#endif

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

    if (tacc_type_is_integral(val->type)) {
        bits = tacc_type_bit_width(compiler->target, val->type->kind);
        alignment = tacc_type_alignment_p2(compiler->target, val->type);

        tacc_compile_output_directive(compiler, "p2align %d", (int) alignment);
        tacc_compile_output(compiler, "\n%s:", tacc_dynstring_as_str(name));

        tacc_compile_output_int(compiler, val->value.int_value, bits);
        return;
    }

    switch (val->type->kind) {
    case TYK_FLOAT:
        tacc_assert(0, "TODO: float support");
        return;
    case TYK_DOUBLE:
        tacc_assert(0, "TODO: float support");
        return;
    case TYK_LONGDOUBLE:
        tacc_assert(0, "TODO: float support");
        return;
    case TYK_VOID:
        tacc_assert(0, "cannot output value of void type");
        return;
    case TYK_PTR:
        tacc_assert(0, "TODO: compile ptr constant");
        break;
    case TYK_STRUCT:
        tacc_assert(0, "TODO: compile struct constant");
        break;
    case TYK_UNION:
        tacc_assert(0, "TODO: compile union constant");
        break;
    case TYK_ENUM:
        tacc_assert(0, "TODO: compile enum constant");
        break;
    case TYK_ARRAY:
        tacc_assert(0, "TODO: compile array constant");
        break;
    case TYK_FN:
        tacc_assert(0, "TODO: compile fn constant (?)");
        break;
    default:
        tacc_assert(0, "unexpected type kind");
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
                    tacc_sub_initializer_list_len(initializer->value.list) == 1,
                    "multiple initializers for a scalar");
                entry =
                    tacc_sub_initializer_list_get(initializer->value.list, 0);
                tacc_assert(entry->content->designator_kind == DESIGNATOR_NONE,
                            "designator used on scalar");
                tacc_assert(entry->content->value->plain_expr,
                            "nested braced initializers for scalar");
                expr = entry->content->value->value.expr;
            } else {
                expr = initializer->value.expr;
            }
            val = tacc_expr_const_eval(
                expr, compiler->target, compiler->basic_types);
            tacc_val_convert(val, for_type->kind, compiler->target);
            val->type = for_type;
        }
        tacc_compile_val(compiler, val, name);
        tacc_val_free(val);
        return;
    }
    tacc_assert(0, "TODO: non-scalar data");
}

static struct tacc_type *
tacc_compiler_find_tagged_type_or_forwdecl(struct tacc_compiler *compiler,
                                           struct tacc_string *name,
                                           enum tacc_type_kind kind) {
    struct tacc_type_map_entry *entry;
    struct tacc_block_scope_list_entry *scope;
    struct tacc_type *ty;
    size_t i;

    /*
     * If forward declaration is encountered and a tagged definition is in
     * scope, the definition must match the declaration in kind. If a
     * definition is given instead, the definition need not match an
     * existing definition or declaration in an outer scope. A definition in an
     * inner scope ignores definitions AND incomplete declarations from outer
     * scopes.
     */
    for (i = tacc_block_scope_list_len(compiler->block_scopes); i > 0;
         i = i - 1) {
        /*
         * Look for nearest declaration/definition. New declarations are not
         * created in surrounding scopes if they match yet outer scopes, so the
         * nearest decl/def is either a definition, or the outermost
         * declaration visible.
         */
        scope = tacc_block_scope_list_get(compiler->block_scopes, i - 1);
        entry = tacc_type_map_get(scope->content->tagged_types,
                                  tacc_dynstring_as_str(name));
        if (entry->content != NULL) {
            tacc_assert(entry->content->kind == kind,
                        "mismatched redeclaration using tag `%s`",
                        tacc_dynstring_as_str(name));

            return entry->content;
        }
    }

    ty = tacc_type_new();
    ty->kind = kind;
    ty->name = tacc_dynstring_clone(name);
    tacc_assert(kind == TYK_ENUM, "TODO: forward-declare struct/union");
    ty->extra.enumeration = NULL;

    scope = tacc_block_scope_list_get(
        compiler->block_scopes,
        tacc_block_scope_list_len(compiler->block_scopes) - 1);
    tacc_type_map_insert(scope->content->tagged_types, ty);

    return ty;
}

static struct tacc_type *tacc_compiler_add_new_tagged_type(
    struct tacc_compiler *compiler, struct tacc_type *ty) {
    struct tacc_block_scope_list_entry *entry;
    struct tacc_type_map_entry *existing_entry;

    entry = tacc_block_scope_list_get(
        compiler->block_scopes,
        tacc_block_scope_list_len(compiler->block_scopes) - 1);
    existing_entry = tacc_type_map_get(entry->content->tagged_types,
                                       tacc_dynstring_as_str(ty->name));
    if (existing_entry != NULL) {
        /* TODO: merge attributes? */
        if (existing_entry->content->kind == TYK_ENUM) {
            tacc_assert(existing_entry->content->extra.enumeration,
                        "redefinition of enum %s",
                        ty);
            existing_entry->content->extra.enumeration = ty->extra.enumeration;
            ty->extra.enumeration = NULL;
        } else {
            tacc_assert(0, "TODO: detect redefinition of struct/union %s", ty);
        }
        tacc_type_free(ty);
        return existing_entry->content;
    }
    tacc_type_map_insert(entry->content->tagged_types, ty);

    return ty;
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
            tacc_assert(tacc_val_is_integral(val),
                        "enumerator must be integer constant");
            if (tacc_val_is_negative(val)) {
                use_negative = 1;
            }
            tacc_assert(tacc_u64_sge(val->value.int_value,
                                     compiler->target->sint->min) &&
                            tacc_u64_sle(val->value.int_value,
                                         compiler->target->sint->max),
                        "enumerator value out of range");
        } else {
            tacc_assert(counter.low != 0xFFFFFFFF || counter.high != 0xFFFFFFFF,
                        "enumerator overflow when implicitly incrementing");
            tacc_u64_add_u32(&counter, &counter, 1);
        }
    }

    if (use_negative) {
        return tacc_get_basic_type(compiler->basic_types, TYK_SINT);
    }
    return tacc_get_basic_type(compiler->basic_types, TYK_UINT);
}

static tacc_bool
tacc_declarator_is_modified(struct tacc_declarator *declarator) {
    struct tacc_declarator *curr;

    curr = declarator;
    while (1) {
        switch (curr->kind) {
        case DECLARATOR_PLAIN:
        case DECLARATOR_ABSTRACT:
            return 0;
        case DECLARATOR_SUB:
            curr = curr->extra.sub_declarator;
            break;
        case DECLARATOR_ARRAY:
            return 1;
        case DECLARATOR_FUNC:
            return 1;
        }
    }
}

static struct tacc_type *
tacc_type_adjust_from_declarator(struct tacc_compiler *compiler,
                                 struct tacc_type *base_type,
                                 struct tacc_declarator *declarator,
                                 struct tacc_string_list *def_param_names);

static void
tacc_type_adjust_function(struct tacc_compiler *compiler,
                          struct tacc_function_type *ty,
                          struct tacc_function_declarator *declarator,
                          struct tacc_string_list *def_param_names) {
    size_t i;
    tacc_bool this_is_def_function_prototype =
        (def_param_names != NULL) &&
        !tacc_declarator_is_modified(declarator->sub_declarator);
    struct tacc_function_param_list_entry *entry;
    struct tacc_string *param_name;
    struct tacc_string_list_entry *param_entry;

    ty->is_vararg = 0;

    switch (declarator->param_list_kind) {
    case FUNCPARAM_LIST_VARARG:
        ty->is_vararg = 1;
        tacc_assert(
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
            tacc_type_list_push(ty->param_types,
                                tacc_type_adjust_from_declarator(
                                    compiler,
                                    tacc_type_from_decl_type(
                                        compiler, entry->content->base_type),
                                    entry->content->decl,
                                    0));
        }
        if (this_is_def_function_prototype) {
            tacc_assert(tacc_string_list_len(def_param_names) == 0,
                        "ICE: found two param lists for function definition?");
            for (i = 0; i < tacc_function_param_list_len(
                                declarator->param_list.modern_params);
                 i = i + 1) {
                entry = tacc_function_param_list_get(
                    declarator->param_list.modern_params, i);
                param_name = tacc_declarator_name(entry->content->decl);
                tacc_assert(
                    param_name != NULL,
                    "abstract declarator in parameter list of function definition");
                tacc_string_list_push(def_param_names, param_name);
            }
        }
        break;
    case FUNCPARAM_VOID:
        /* function specified to take no parameters */
        ty->param_types = tacc_type_list_new();
        break;
    case FUNCPARAM_EMPTY_LIST:
        if (this_is_def_function_prototype) {
            /*
             * when specifying the argument list of a function being defined,
             * this is equivalent to void
             */
            ty->param_types = tacc_type_list_new();
        } else {
            /* function type declarator with unspecified parameter types */
            ty->param_types = NULL;
        }
        break;
    case FUNCPARAM_OLD_STYLE_LIST:
        tacc_assert(this_is_def_function_prototype,
                    "old-style parameter list outside function definition");
        tacc_assert(tacc_string_list_len(def_param_names) == 0,
                    "ICE: found two param lists for function definition?");
        ty->param_types = NULL;
        for (i = 0;
             i < tacc_string_list_len(declarator->param_list.old_style_params);
             i = i + 1) {
            param_entry = tacc_string_list_get(
                declarator->param_list.old_style_params, i);
            tacc_string_list_push(def_param_names, param_entry->content);
        }
        break;
    }
}

static struct tacc_type *
tacc_type_adjust_from_declarator(struct tacc_compiler *compiler,
                                 struct tacc_type *base_type,
                                 struct tacc_declarator *declarator,
                                 struct tacc_string_list *def_param_names) {
    struct tacc_declarator *curr_declarator;
    struct tacc_type *curr_type;
    struct tacc_type *sub_type;
    struct tacc_val *dimension;

    curr_declarator = declarator;
    curr_type = base_type;

    while (1) {
        if (curr_declarator->kind == DECLARATOR_PLAIN ||
            curr_declarator->kind == DECLARATOR_ABSTRACT) {
            return tacc_type_to_pointer(curr_type,
                                        curr_declarator->indirection_level);
        }
        if (curr_declarator->kind == DECLARATOR_SUB) {
            curr_type = tacc_type_to_pointer(
                curr_type, curr_declarator->indirection_level);
            curr_declarator = curr_declarator->extra.sub_declarator;
            continue;
        }
        if (curr_declarator->kind == DECLARATOR_ARRAY) {
            sub_type = tacc_type_to_pointer(curr_type,
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
                    tacc_assert(tacc_val_is_integral(dimension),
                                "array dimension must be an integer");
                    tacc_assert(!tacc_val_is_negative(dimension),
                                "array dimension must be nonnegative");
                    tacc_val_convert(dimension, TYK_UINT, compiler->target);
                    curr_type->extra.array->dimension =
                        dimension->value.int_value;
                } else {
                    curr_type->kind = TYK_VLA;
                }
            }
            tacc_type_list_push(sub_type->derived_array_types, curr_type);
            curr_declarator = curr_declarator->extra.arr_decl->sub_declarator;
            continue;
        }
        /* function declarator */
        sub_type =
            tacc_type_to_pointer(curr_type, curr_declarator->indirection_level);
        curr_type = tacc_type_new();
        curr_type->kind = TYK_FN;
        tacc_type_list_push(sub_type->derived_func_types, curr_type);
        curr_type->extra.function = tacc_function_type_new();
        curr_type->extra.function->return_type = sub_type;
        tacc_type_adjust_function(compiler,
                                  curr_type->extra.function,
                                  curr_declarator->extra.func_decl,
                                  def_param_names);
        curr_declarator = curr_declarator->extra.func_decl->sub_declarator;
    }
}

static void tacc_struct_push_field(struct tacc_compiler *compiler,
                                   struct tacc_struct_type *ty,
                                   struct tacc_field *field) {
    size_t alignment;

    alignment = tacc_type_alignment_p2(compiler->target, field->type);
    if (alignment < ty->alignment_p2) {
        ty->alignment_p2 = alignment;
    }

    tacc_field_list_push(ty->fields, field);
}
static void tacc_union_push_field(struct tacc_compiler *compiler,
                                  struct tacc_union_type *ty,
                                  struct tacc_field *field) {
    size_t alignment;

    alignment = tacc_type_alignment_p2(compiler->target, field->type);
    if (alignment < ty->alignment_p2) {
        ty->alignment_p2 = alignment;
    }

    tacc_field_list_push(ty->fields, field);
}

static size_t tacc_align(size_t x, size_t alignment_p2) {
    return x & ~((size_t) ((1 << alignment_p2) - 1));
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
                compiler, base_ty, declarator_entry->content->underlying, NULL);
            field = tacc_field_new();
            field->type = adjusted_ty;
            field->name =
                tacc_declarator_name(declarator_entry->content->underlying);
            if (declarator_entry->content->bitfield_size == NULL) {
                bit_offset = tacc_align(
                    bit_offset,
                    3 + tacc_type_alignment_p2(compiler->target, adjusted_ty));
                field->offset = bit_offset >> 3;
                tacc_struct_push_field(compiler, ty, field);
                bit_offset =
                    bit_offset +
                    (tacc_type_size(compiler->target, adjusted_ty) << 3);
                continue;
            } else {
                tacc_assert(0, "TODO: evaluate bitfields in structures");
            }
        }
    }
    ty->size = tacc_align(bit_offset >> 3, ty->alignment_p2);

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
                compiler, base_ty, declarator_entry->content->underlying, NULL);
            field = tacc_field_new();
            field->type = adjusted_ty;
            field->name =
                tacc_declarator_name(declarator_entry->content->underlying);
            field->offset = 0;
            if (declarator_entry->content->bitfield_size == NULL) {
                tacc_union_push_field(compiler, ty, field);
                continue;
            } else {
                tacc_assert(0, "TODO: evaluate bitfields in unions");
            }
        }
    }

    return ty;
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
            tacc_assert(type->referenced_name != NULL,
                        "anonymous unspecified enumeration");
            ty = tacc_compiler_find_tagged_type_or_forwdecl(
                compiler, type->referenced_name, TYK_ENUM);
        } else {
            ty = tacc_type_new();
            ty->kind = TYK_ENUM;
            ty->extra.enumeration = tacc_enumeration_type_new();
            ty->extra.enumeration->underlying_type =
                tacc_eval_enumerators(compiler, type->extra.enumerators);
            if (type->referenced_name != NULL) {
                ty->name = tacc_dynstring_clone(type->referenced_name);
                /*
                 * May result in merge of past forward-declaration,
                 * returning the tacc_type from that forward-declaration.
                 */
                ty = tacc_compiler_add_new_tagged_type(compiler, ty);
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
            tacc_assert(type->referenced_name != NULL,
                        "anonymous unspecified struct/union");
            ty = tacc_compiler_find_tagged_type_or_forwdecl(
                compiler, type->referenced_name, base_type);
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
            if (type->referenced_name != NULL) {
                ty->name = tacc_dynstring_clone(type->referenced_name);
                /*
                 * May result in merge of past forward-declaration,
                 * returning the tacc_type from that forward-declaration.
                 */
                ty = tacc_compiler_add_new_tagged_type(compiler, ty);
            } else {
                tacc_type_list_push(compiler->anonymous_types, ty);
            }
        }
        break;
    case TYPESPEC_TYPEDEF:
        tacc_assert(0, "TODO: construct type from typedef");
        return NULL;
    default:
        tacc_assert(0, "type unsupported as of now");
        return NULL;
    }

    entry = tacc_type_list_get(compiler->basic_types, base_type);
    ty = entry->content;

    return ty;
}

struct tacc_block_scope *tacc_block_scope_new(void) {
    struct tacc_block_scope *scope;

    scope = tacc_malloc(sizeof(struct tacc_block_scope));
    scope->tagged_types = tacc_type_map_new(0x1000);
    scope->untagged_idents = tacc_compile_ident_map_new(0x1000);

    return scope;
}

void tacc_block_scope_free(struct tacc_block_scope *scope) {
    tacc_type_map_free(scope->tagged_types);
    tacc_free(scope->tagged_types);
    tacc_compile_ident_map_free(scope->untagged_idents);
    tacc_free(scope->untagged_idents);
    tacc_free(scope);
}

void tacc_compile_ident_free(struct tacc_compile_ident *ident) {
    tacc_dynstring_free(ident->ident);
    tacc_free(ident);
}

static void tacc_compile_function_def(struct tacc_compiler *compiler,
                                      struct tacc_decl *function_def) {
    struct tacc_string_list *param_list;
    struct tacc_type *function_type;
    struct tacc_codegen_state *state;

    param_list = tacc_string_list_new();
    function_type = tacc_type_adjust_from_declarator(
        compiler,
        tacc_type_from_decl_type(compiler, function_def->base_type),
        function_def->extra.func_def->func_declaration,
        param_list);
    tacc_assert(function_def->extra.func_def->old_style_param_list == NULL,
                "TODO: old-style function parameter types");

#ifndef __M2__
    (void) function_type;
#endif

    state = tacc_codegen_state_new(compiler->target, compiler->basic_types);
    tacc_codegen_compile_statements(state,
                                    function_def->extra.func_def->statements);

    tacc_compile_output_directive(compiler, "section .text, \"ax\", @progbits");
    tacc_compile_output_directive(
        compiler,
        "globl %s",
        tacc_dynstring_as_str(tacc_declarator_name(
            function_def->extra.func_def->func_declaration)));
    tacc_compile_output(compiler,
                        "\n%s:",
                        tacc_dynstring_as_str(tacc_declarator_name(
                            function_def->extra.func_def->func_declaration)));
    tacc_compile_output(
        compiler, "%s", tacc_dynstring_as_str(state->code_buffer));
    tacc_compile_output(compiler, "\n.Lepilog:");
    tacc_compile_output(compiler, "\n\t ret\n");

    tacc_codegen_state_free(state);
    state = NULL;

    /* free collected param_list */
    tacc_string_list_free(param_list);
    tacc_free(param_list);
}

void tacc_compile_prelude(struct tacc_compiler *compiler) {
    tacc_target_codegen_prelude(compiler);
}

void tacc_compile_top_decl(struct tacc_compiler *compiler,
                           struct tacc_decl *decl) {
    struct tacc_type *type;
    size_t i;
    struct tacc_init_declarator_list_entry *entry;
    struct tacc_init_declarator *declarator;
    struct tacc_string *data_name;

    if (decl->kind == DECL_FUNCTION_DEF) {
        tacc_compile_function_def(compiler, decl);
        return;
    }
    tacc_assert(decl->kind == DECL_DECLARATORS, "TODO: function definition");
    tacc_assert(decl->storage_class == STORAGE_UNSPECIFIED,
                "TODO: different storage classes");
    type = tacc_type_from_decl_type(compiler, decl->base_type);
    for (i = 0; i < tacc_init_declarator_list_len(decl->extra.declarators);
         i = i + 1) {
        entry = tacc_init_declarator_list_get(decl->extra.declarators, i);
        data_name = tacc_declarator_name(entry->content->declarator);
        declarator = entry->content;
        tacc_assert(declarator->declarator->kind == DECLARATOR_PLAIN,
                    "TODO: non-plain declarator");
        tacc_compile_data(compiler, type, data_name, declarator->initializer);
    }
    tacc_compile_output(compiler, "\n");
}

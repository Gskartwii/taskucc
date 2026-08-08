#include "compile.h"
#include "decl.h"
#include "expr.h"
#include "machine.h"
#include "type.h"
#include <stdarg.h>

static void tacc_compile_output_directive(struct tacc_compiler *compiler,
                                          char *directive_fmt,
                                          ...) {
    va_list va;

#ifndef __M2__
    (void) compiler;
#endif

    va_start(va, directive_fmt);
    printf("\n\t.");
    vprintf(directive_fmt, va);
}

static void tacc_compile_output(struct tacc_compiler *compiler,
                                char *fmt,
                                ...) {
    va_list va;

#ifndef __M2__
    (void) compiler;
#endif

    va_start(va, fmt);
    vprintf(fmt, va);
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
    case TYK_ARRAY_FLEX:
        tacc_assert(0, "TODO: compile flexible array constant");
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
    default:
        tacc_assert(0, "type unsupported as of now");
        return NULL;
    }

    entry = tacc_type_list_get(compiler->basic_types, base_type);
    ty = entry->content;

    return ty;
}

void tacc_compile_top_decl(struct tacc_compiler *compiler,
                           struct tacc_decl *decl) {
    struct tacc_type *type;
    size_t i;
    struct tacc_init_declarator_list_entry *entry;
    struct tacc_init_declarator *declarator;
    struct tacc_string *data_name;

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

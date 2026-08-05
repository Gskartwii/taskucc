#include "compile.h"
#include "decl.h"
#include "expr.h"
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
                                    size_t bits,
                                    size_t alignment) {
    size_t i;
    struct tacc_u64 aux;

    tacc_compile_output_directive(compiler, "p2align %d", (int) alignment);
    tacc_compile_output_directive(compiler, "byte ");

    for (i = 0; i < bits; i = i + 8) {
        tacc_u64_rsh_n(&aux, int_val, (int) i);
    }
}

static void tacc_compile_val(struct tacc_compiler *compiler,
                             struct tacc_val *val) {
    size_t bits;
    size_t alignment;

    if (tacc_type_is_integral(val->type)) {
        bits = tacc_type_bit_width(compiler->target, val->type->kind);
        alignment = tacc_type_alignment_p2(compiler->target, val->type);

        tacc_compile_output_int(
            compiler, val->value.int_value, bits, alignment);
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
                              struct tacc_initializer *initializer) {
    struct tacc_sub_initializer_list_entry *entry;
    struct tacc_expr *expr;
    struct tacc_val *val;

    if (tacc_type_is_scalar(for_type)) {
        if (!initializer->plain_expr) {
            tacc_assert(
                tacc_sub_initializer_list_len(initializer->value.list) == 1,
                "multiple initializers for a scalar");
            entry = tacc_sub_initializer_list_get(initializer->value.list, 0);
            tacc_assert(entry->content->designator_kind == DESIGNATOR_NONE,
                        "designator used on scalar");
            tacc_assert(entry->content->value->plain_expr,
                        "nested braced initializers for scalar");
            expr = entry->content->value->value.expr;
        } else {
            expr = initializer->value.expr;
        }
        val =
            tacc_expr_const_eval(expr, compiler->target, compiler->basic_types);
        tacc_val_convert(val, for_type->kind, compiler->target);
        tacc_compile_val(compiler, val);
        return;
    }
    tacc_assert(0, "TODO: non-scalar data");
}

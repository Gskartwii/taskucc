#include "expr.h"
#include "target_defs.h"
#include "type.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void tacc_expr_init(struct tacc_expr *expr) {
    expr->op1 = NULL;
    expr->op2 = NULL;
    expr->op3 = NULL;
    expr->kind = EX_UNINIT;
}

struct tacc_expr *tacc_expr_new(void) {
    struct tacc_expr *expr = tacc_malloc(sizeof(struct tacc_expr));
    tacc_expr_init(expr);
    return expr;
}

void tacc_expr_free(struct tacc_expr *expr) {
    if (expr->op1) {
        tacc_expr_free(expr->op1);
    }
    if (expr->op2) {
        tacc_expr_free(expr->op2);
    }
    if (expr->op3) {
        tacc_expr_free(expr->op3);
    }
    if (expr->kind == EX_NUM_LIT) {
        tacc_val_free(expr->extra.const_val);
    }
    tacc_free(expr);
}

MK_DYNARRAY_OVER(tacc_expr_list,
                 tacc_expr_list_entry,
                 struct tacc_expr *,
                 tacc_expr_list_new,
                 tacc_expr_list_init,
                 tacc_expr_list_get,
                 tacc_expr_list_push,
                 tacc_expr_list_pop,
                 tacc_expr_list_len,
                 tacc_expr_free,
                 tacc_expr_list_free)

struct tacc_val *tacc_expr_const_eval(struct tacc_expr *expr,
                                      struct tacc_target *target) {
    struct tacc_val *l_result;
    struct tacc_val *r_result;

    switch (expr->kind) {
    case EX_UNINIT:
        tacc_assert(0, "invalid uninitialized expr");
        break;
    case EX_NUM_LIT:
        return tacc_val_clone(expr->extra.const_val);
    case EX_STRING_LIT:
        tacc_assert(0, "todo: string consteval");
        break;
    case EX_IDENT:
        tacc_assert(0, "todo: ident consteval");
        break;
    case EX_ADD:
        tacc_assert(0, "todo: add consteval");
        break;
    case EX_SUB:
        tacc_assert(0, "todo: sub consteval");
        break;
    case EX_MUL:
        tacc_assert(0, "todo: mul consteval");
        break;
    case EX_DIV:
        tacc_assert(0, "todo: div consteval");
        break;
    case EX_REM:
        tacc_assert(0, "todo: rem consteval");
        break;
    case EX_POS:
        tacc_assert(0, "todo: pos consteval");
        break;
    case EX_NEG:
        tacc_assert(0, "todo: neg consteval");
        break;
    case EX_BAND:
        tacc_assert(0, "todo: & consteval");
        break;
    case EX_BOR:
        tacc_assert(0, "todo: | consteval");
        break;
    case EX_BXOR:
        tacc_assert(0, "todo: ^ consteval");
        break;
    case EX_BNOT:
        tacc_assert(0, "todo: ~ consteval");
        break;
    case EX_SHL:
        tacc_assert(0, "todo: << consteval");
        break;
    case EX_SHR:
        tacc_assert(0, "todo: >> consteval");
        break;
    case EX_AND:
        l_result = tacc_expr_const_eval(expr->op1, target);
        tacc_assert(tacc_val_is_scalar(l_result), "&& takes a scalar operand");
        if (!tacc_val_is_truthy(l_result)) {
            tacc_val_free(l_result);
            return tacc_val_from_int(0, TYK_SINT, target);
        }
        tacc_val_free(l_result);
        l_result = NULL;

        r_result = tacc_expr_const_eval(expr->op2, target);
        if (!tacc_val_is_truthy(r_result)) {
            tacc_val_free(r_result);
            return tacc_val_from_int(0, TYK_SINT, target);
        }
        tacc_val_free(r_result);
        r_result = NULL;

        return tacc_val_from_int(1, TYK_SINT, target);
    case EX_OR:
        l_result = tacc_expr_const_eval(expr->op1, target);
        tacc_assert(tacc_val_is_scalar(l_result), "|| takes a scalar operand");
        if (tacc_val_is_truthy(l_result)) {
            tacc_val_free(l_result);
            return tacc_val_from_int(1, TYK_SINT, target);
        }
        tacc_val_free(l_result);
        l_result = NULL;

        r_result = tacc_expr_const_eval(expr->op2, target);
        if (tacc_val_is_truthy(r_result)) {
            tacc_val_free(r_result);
            return tacc_val_from_int(1, TYK_SINT, target);
        }
        tacc_val_free(r_result);
        r_result = NULL;

        return tacc_val_from_int(0, TYK_SINT, target);
    case EX_NOT:
        l_result = tacc_expr_const_eval(expr->op1, target);
        tacc_assert(tacc_val_is_scalar(l_result), "! takes a scalar operand");
        if (tacc_val_is_truthy(l_result)) {
            tacc_val_free(l_result);
            return tacc_val_from_int(0, TYK_SINT, target);
        }
        tacc_val_free(l_result);
        return tacc_val_from_int(1, TYK_SINT, target);
    case EX_EQ:
        l_result = tacc_expr_const_eval(expr->op1, target);
        r_result = tacc_expr_const_eval(expr->op2, target);
        tacc_assert(tacc_val_is_arithmetic(l_result) &&
                        tacc_val_is_arithmetic(r_result),
                    "todo: non-arithmetic eq consteval");
        tacc_val_usual_arithmetic_conversions(l_result, r_result, target);
        if (!tacc_val_is_eq(l_result, r_result)) {
            tacc_val_free(l_result);
            tacc_val_free(r_result);
            return tacc_val_from_int(0, TYK_SINT, target);
        }
        tacc_val_free(l_result);
        tacc_val_free(r_result);
        return tacc_val_from_int(1, TYK_SINT, target);
        break;
    case EX_NE:
        l_result = tacc_expr_const_eval(expr->op1, target);
        r_result = tacc_expr_const_eval(expr->op2, target);
        tacc_assert(tacc_val_is_arithmetic(l_result) &&
                        tacc_val_is_arithmetic(r_result),
                    "todo: non-arithmetic ne consteval");
        tacc_val_usual_arithmetic_conversions(l_result, r_result, target);
        if (tacc_val_is_eq(l_result, r_result)) {
            tacc_val_free(l_result);
            tacc_val_free(r_result);
            return tacc_val_from_int(0, TYK_SINT, target);
        }
        tacc_val_free(l_result);
        tacc_val_free(r_result);
        return tacc_val_from_int(1, TYK_SINT, target);
        break;
    case EX_LE:
        tacc_assert(0, "todo: <= consteval");
        break;
    case EX_LT:
        tacc_assert(0, "todo: < consteval");
        break;
    case EX_GE:
        tacc_assert(0, "todo: >= consteval");
        break;
    case EX_GT:
        tacc_assert(0, "todo: > consteval");
        break;
    case EX_ASSI:
        tacc_assert(0, "todo: = consteval");
        break;
    case EX_ADD_ASSI:
        tacc_assert(0, "todo: += consteval");
        break;
    case EX_SUB_ASSI:
        tacc_assert(0, "todo: -= consteval");
        break;
    case EX_MUL_ASSI:
        tacc_assert(0, "todo: *= consteval");
        break;
    case EX_DIV_ASSI:
        tacc_assert(0, "todo: /= consteval");
        break;
    case EX_REM_ASSI:
        tacc_assert(0, "todo: %= consteval");
        break;
    case EX_BAND_ASSI:
        tacc_assert(0, "todo: &= consteval");
        break;
    case EX_BOR_ASSI:
        tacc_assert(0, "todo: |= consteval");
        break;
    case EX_BXOR_ASSI:
        tacc_assert(0, "todo: ^= consteval");
        break;
    case EX_LSH_ASSI:
        tacc_assert(0, "todo: <<= consteval");
        break;
    case EX_RSH_ASSI:
        tacc_assert(0, "todo: >>= consteval");
        break;
    case EX_INCR_PRE:
        tacc_assert(0, "todo: ++pre consteval");
        break;
    case EX_DECR_PRE:
        tacc_assert(0, "todo: --pre consteval");
        break;
    case EX_INCR_POST:
        tacc_assert(0, "todo: post++ consteval");
        break;
    case EX_DECR_POST:
        tacc_assert(0, "todo: post-- consteval");
        break;
    case EX_SUBSCRIPT:
        tacc_assert(0, "todo: _[_] consteval");
        break;
    case EX_DEREF:
        tacc_assert(0, "todo: deref consteval");
        break;
    case EX_ADDROF:
        tacc_assert(0, "todo: ampersand consteval");
        break;
    case EX_MEMBER:
        tacc_assert(0, "todo: _._ consteval");
        break;
    case EX_PTR_MEMBER:
        tacc_assert(0, "todo: _->_ consteval");
        break;
    case EX_CALL:
        tacc_assert(0, "todo: _(_) consteval");
        break;
    case EX_COMMA:
        tacc_assert(0, "todo: _,_ consteval");
        break;
    case EX_CAST:
        tacc_assert(0, "todo: (_)_ consteval");
        break;
    case EX_SIZEOF:
        tacc_assert(0, "todo: sizeof _ consteval");
        break;
    case EX_SIZEOF_TY:
        tacc_assert(0, "todo: sizeof(_ty) consteval");
        break;
    case EX_SELECT:
        tacc_assert(0, "todo: selection consteval");
        break;
    case EX_COMPOUND_LIT:
        tacc_assert(0, "todo: (_){_} consteval");
        break;
    }
    return NULL;
}

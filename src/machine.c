#include "machine.h"
#include "soft_u64.h"
#include "type.h"
#include <string.h>

struct tacc_val *tacc_val_new(void) {
    struct tacc_val *val = tacc_malloc(sizeof(struct tacc_val));

    val->type = NULL;

    return val;
}

struct tacc_val *tacc_val_clone(struct tacc_val *orig_val) {
    struct tacc_val *val = tacc_malloc(sizeof(struct tacc_val));

    val->type = orig_val->type;

    if (tacc_val_is_integral(orig_val)) {
        val->value.int_value = tacc_u64_clone(orig_val->value.int_value);
    } else {
        tacc_assert(0, "non-integral vals");
    }

    return val;
}

tacc_bool tacc_val_is_integral(struct tacc_val *val) {
    switch (val->type->kind) {
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

tacc_bool tacc_val_is_signed(struct tacc_val *val) {
    return tacc_type_kind_is_signed(val->type->kind);
}

tacc_bool tacc_val_is_scalar(struct tacc_val *val) {
    return tacc_type_is_scalar(val->type);
}

tacc_bool tacc_val_is_arithmetic(struct tacc_val *val) {
    switch (val->type->kind) {
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
    case TYK_ENUM:
        return 1;
    default:
        return 0;
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

void tacc_val_convert(struct tacc_val *val,
                      enum tacc_type_kind into,
                      struct tacc_target *target) {
    if (tacc_type_is_subset(val->type->kind, into, target)) {
        /* already fits into destination type */
        return;
    }
    if (tacc_type_kind_is_signed(into)) {
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

    a_type = a->type->kind;
    b_type = b->type->kind;

    tacc_assert(tacc_val_is_integral(a) && tacc_val_is_integral(b),
                "TODO: arith conversions for non-integral types");

    if (a_type == b_type) {
        return;
    }

    a_rank = tacc_type_rank(a_type);
    b_rank = tacc_type_rank(b_type);

    if (tacc_val_is_signed(a) == tacc_val_is_signed(b)) {
        /* same signedness => convert to the higher-ranked of the types */
        if (a_rank <= b_rank) {
            tacc_val_convert(a, b_type, target);
        } else {
            tacc_val_convert(b, a_type, target);
        }
        return;
    }
    if (!tacc_val_is_signed(a) && (b_rank <= a_rank)) {
        tacc_val_convert(b, a_type, target);
    } else if (!tacc_val_is_signed(b) && (a_rank <= b_rank)) {
        tacc_val_convert(a, b_type, target);
    } else if (tacc_val_is_signed(a) &&
               tacc_type_is_subset(b_type, a_type, target)) {
        tacc_val_convert(b, a_type, target);
    } else if (tacc_val_is_signed(b) &&
               tacc_type_is_subset(a_type, b_type, target)) {
        tacc_val_convert(a, b_type, target);
    } else {
        if (tacc_val_is_signed(b)) {
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

struct tacc_val *tacc_val_from_int(int value, struct tacc_type *ty) {
    struct tacc_val *val;

    val = tacc_val_new();
    val->type = ty;
    val->value.int_value = tacc_u64_new_from_u32((uint32_t) value);
    if (tacc_val_is_signed(val) && (value < 0)) {
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

struct tacc_val *tacc_val_zero(struct tacc_type *ty) {
    struct tacc_val *val;

    tacc_assert(tacc_type_is_integral(ty),
                "TODO: zero values for non-integral types");
    val = tacc_val_new();
    val->type = ty;
    val->value.int_value = tacc_u64_new();

    return val;
}

static struct tacc_val *
tacc_int_literal_eval(struct tacc_int_literal *literal,
                      struct tacc_target *target,
                      struct tacc_type_list *basic_types) {
    struct tacc_val *val;
    tacc_bool can_be_unsigned = literal->suffix_u || (literal->base != 10);

    if (tacc_u64_ugt(literal->number, target->sllong->max)) {
        tacc_assert(can_be_unsigned, "integer literal overflow");
    }

    val = tacc_val_new();
    val->value.int_value = tacc_u64_clone(literal->number);

    if (literal->suffix_ll) {
        if (literal->suffix_u) {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
            return val;
        }
        if (tacc_u64_ugt(literal->number, target->sllong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
            return val;
        }
        val->type = tacc_get_basic_type(basic_types, TYK_SLONGLONG);
        return val;
    }
    if (literal->suffix_l) {
        if (literal->suffix_u) {
            if (tacc_u64_ule(literal->number, target->ulong->max)) {
                val->type = tacc_get_basic_type(basic_types, TYK_ULONG);
                return val;
            }
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
        } else if (tacc_u64_ule(literal->number, target->slong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_SLONG);
        } else if (can_be_unsigned &&
                   tacc_u64_ule(literal->number, target->ulong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONG);
        } else if (tacc_u64_ule(literal->number, target->sllong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_SLONGLONG);
        } else {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
        }
        return val;
    }

    if (literal->suffix_u) {
        if (tacc_u64_ule(literal->number, target->uint->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_UINT);
        } else if (tacc_u64_ule(literal->number, target->ulong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONG);
        } else {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
        }
        return val;
    }

    if (tacc_u64_ule(literal->number, target->sint->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_SINT);
    } else if (can_be_unsigned &&
               tacc_u64_ule(literal->number, target->uint->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_UINT);
    } else if (tacc_u64_ule(literal->number, target->slong->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_SLONG);
    } else if (can_be_unsigned &&
               tacc_u64_ule(literal->number, target->ulong->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_ULONG);
    } else if (tacc_u64_ule(literal->number, target->sllong->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_SLONGLONG);
    } else {
        val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
    }
    return val;
}

struct tacc_val *tacc_expr_const_eval(struct tacc_expr *expr,
                                      struct tacc_target *target,
                                      struct tacc_type_list *basic_types) {
    struct tacc_val *l_result;
    struct tacc_val *r_result;
    struct tacc_type *sint_ty = tacc_get_basic_type(basic_types, TYK_SINT);

    switch (expr->kind) {
    case EX_UNINIT:
        tacc_assert(0, "invalid uninitialized expr");
        break;
    case EX_INT_LIT:
        return tacc_int_literal_eval(
            expr->extra.int_literal, target, basic_types);
    case EX_CHAR_LIT:
        tacc_assert(0, "todo: char-literal consteval");
        break;
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
        l_result = tacc_expr_const_eval(expr->op1, target, basic_types);
        tacc_assert(tacc_val_is_scalar(l_result), "&& takes a scalar operand");
        if (!tacc_val_is_truthy(l_result)) {
            tacc_val_free(l_result);
            return tacc_val_from_int(0, sint_ty);
        }
        tacc_val_free(l_result);
        l_result = NULL;

        r_result = tacc_expr_const_eval(expr->op2, target, basic_types);
        if (!tacc_val_is_truthy(r_result)) {
            tacc_val_free(r_result);
            return tacc_val_from_int(0, sint_ty);
        }
        tacc_val_free(r_result);
        r_result = NULL;

        return tacc_val_from_int(1, sint_ty);
    case EX_OR:
        l_result = tacc_expr_const_eval(expr->op1, target, basic_types);
        tacc_assert(tacc_val_is_scalar(l_result), "|| takes a scalar operand");
        if (tacc_val_is_truthy(l_result)) {
            tacc_val_free(l_result);
            return tacc_val_from_int(1, sint_ty);
        }
        tacc_val_free(l_result);
        l_result = NULL;

        r_result = tacc_expr_const_eval(expr->op2, target, basic_types);
        if (tacc_val_is_truthy(r_result)) {
            tacc_val_free(r_result);
            return tacc_val_from_int(1, sint_ty);
        }
        tacc_val_free(r_result);
        r_result = NULL;

        return tacc_val_from_int(0, sint_ty);
    case EX_NOT:
        l_result = tacc_expr_const_eval(expr->op1, target, basic_types);
        tacc_assert(tacc_val_is_scalar(l_result), "! takes a scalar operand");
        if (tacc_val_is_truthy(l_result)) {
            tacc_val_free(l_result);
            return tacc_val_from_int(0, sint_ty);
        }
        tacc_val_free(l_result);
        return tacc_val_from_int(1, sint_ty);
    case EX_EQ:
        l_result = tacc_expr_const_eval(expr->op1, target, basic_types);
        r_result = tacc_expr_const_eval(expr->op2, target, basic_types);
        tacc_assert(tacc_val_is_arithmetic(l_result) &&
                        tacc_val_is_arithmetic(r_result),
                    "todo: non-arithmetic eq consteval");
        tacc_val_usual_arithmetic_conversions(l_result, r_result, target);
        if (!tacc_val_is_eq(l_result, r_result)) {
            tacc_val_free(l_result);
            tacc_val_free(r_result);
            return tacc_val_from_int(0, sint_ty);
        }
        tacc_val_free(l_result);
        tacc_val_free(r_result);
        return tacc_val_from_int(1, sint_ty);
    case EX_NE:
        l_result = tacc_expr_const_eval(expr->op1, target, basic_types);
        r_result = tacc_expr_const_eval(expr->op2, target, basic_types);
        tacc_assert(tacc_val_is_arithmetic(l_result) &&
                        tacc_val_is_arithmetic(r_result),
                    "todo: non-arithmetic ne consteval");
        tacc_val_usual_arithmetic_conversions(l_result, r_result, target);
        if (tacc_val_is_eq(l_result, r_result)) {
            tacc_val_free(l_result);
            tacc_val_free(r_result);
            return tacc_val_from_int(0, sint_ty);
        }
        tacc_val_free(l_result);
        tacc_val_free(r_result);
        return tacc_val_from_int(1, sint_ty);
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

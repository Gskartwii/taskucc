#include "expr.h"
#include "3rdparty/intscan.h"
#include "dynstring.h"
#include "soft_u64.h"
#include "target_defs.h"
#include "tasku_pp.h"
#include "type.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void tacc_expr_init(struct tacc_expr *expr) {
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

static void tacc_expr_bump_to_op1(struct tacc_expr *expr) {
    struct tacc_expr *new_expr;

    new_expr = tacc_expr_new();
    memcpy(new_expr, expr, sizeof(struct tacc_expr));
    tacc_expr_init(expr);
    expr->op1 = new_expr;
    expr->extra.const_val = NULL;
    expr->extra.name = NULL;
    expr->extra.op_list = NULL;
    expr->extra.type = NULL;
}

static void tacc_parse_assert(struct tacc_tok_iter *iter,
                              tacc_bool cond,
                              char *msg) {
    if (cond) {
        return;
    }
    if (!iter->file_iter->filename) {
        printf("in #if %s:\n", iter->file_iter->src);
    } else {
        printf("in %s:\n", iter->file_iter->filename);
    }
    tacc_assert(0, "%s", msg);
}

static void tacc_parse_error(struct tacc_tok_iter *iter, char *msg, ...) {
    va_list va;
    va_start(va, msg);

    if (!iter->file_iter->filename) {
        printf("in #if %s:\n", iter->file_iter->src);
    } else {
        printf("in %s:\n", iter->file_iter->filename);
    }
    vprintf(msg, va);
    printf("\n");
    tacc_assert(0, "parse error", msg);
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

static tacc_bool tacc_tok_gives_typename(struct pp_tok *tok) {
    if (tok->kind != TOK_IDENT) {
        return 0;
    }
    switch (tok->ident_kind) {
    case ID_TYPEDEF_NAME:
    case ID_CONST:
    case ID_RESTRICT:
    case ID_VOLATILE:
    case ID_ENUM:
    case ID_STRUCT:
    case ID_UNION:
    case ID_VOID:
    case ID_SHORT:
    case ID_INT:
    case ID_LONG:
    case ID_FLOAT:
    case ID_DOUBLE:
    case ID_SIGNED:
    case ID_UNSIGNED:
    case ID__BOOL:
        return 1;
    default:
        return 0;
    }
}

static struct tacc_type *tacc_type_parse(struct tacc_tok_iter *iter) {
#ifdef __STDC__
    (void) iter;
#endif
    tacc_parse_error(iter, "todo: type parser");
    return 0;
}

static tacc_bool tacc_tok_non_kw_ident(struct pp_tok *tok) {
    return tok->kind == TOK_IDENT &&
           (tok->ident_kind == ID_OTHER || tok->ident_kind == ID_TYPEDEF_NAME);
}

static struct tacc_val *tacc_parse_numlit(struct tacc_target *target,
                                          struct pp_tok *tok) {
    struct tacc_val *val;
    struct tacc_u64 *u64;
    char *cstr;
    char *cstr_last;
    size_t len;
    unsigned int base;
    int count_l;
    tacc_bool specified_u;
    tacc_bool can_be_unsigned;
    struct tacc_file_iter *iter;
    struct tacc_u64 limit;

    val = tacc_val_new();
    tacc_assert(tok->str != NULL, "need str to parse numlit");
    len = tacc_dynstring_len(tok->str);
    cstr = tacc_dynstring_take_str(tok->str);
    tok = NULL;

    tacc_assert(len > 0, "invalid empty ppnumber");

    u64 = tacc_u64_new();
    val->value.int_value = u64;

    if (len == 1) {
        tacc_u64_zero(u64);
        tacc_u64_add_u32(u64, u64, (uint32_t) (*cstr - '0'));
        val->type_kind = TYK_SINT;
        tacc_free(cstr);
        return val;
    }
    cstr_last = cstr + len - 1;

    specified_u = 0;
    count_l = 0;
    if (*cstr_last == 'U' || *cstr_last == 'u') {
        specified_u = 1;
        cstr_last = cstr_last - 1;
    }
    if (*cstr_last == 'l' || *cstr_last == 'L') {
        cstr_last = cstr_last - 1;
        count_l = 1;
        if (*cstr_last == 'l' || *cstr_last == 'L') {
            count_l = 2;
            cstr_last = cstr_last - 1;
        }
    }
    if (!specified_u && (*cstr_last == 'U' || *cstr_last == 'u')) {
        specified_u = 1;
        cstr_last = cstr_last - 1;
    }
    cstr_last = cstr_last + 1;
    *cstr_last = 0;

    iter = tacc_file_iter_new_str(cstr, cstr_last);

    base = 10;
    if (tacc_file_iter_accept_ch(iter, '0')) {
        base = 8;
        if (tacc_file_iter_accept_ch(iter, 'x')) {
            base = 16;
        } else if (tacc_file_iter_accept_ch(iter, 'X')) {
            base = 16;
        }
    }

    can_be_unsigned = specified_u || (base != 10);
    if (can_be_unsigned) {
        tacc_u64_copy(&limit, target->sllong->max);
    } else {
        tacc_u64_copy(&limit, target->ullong->max);
    }
    intscan(iter, base, &limit, u64);

    tacc_file_iter_free(iter);

    if (count_l == 2) {
        if (specified_u) {
            val->type_kind = TYK_ULONGLONG;
            return val;
        }
        if (tacc_u64_ugt(u64, target->sllong->max)) {
            val->type_kind = TYK_ULONGLONG;
            return val;
        }
        val->type_kind = TYK_SLONGLONG;
        return val;
    }
    if (count_l == 1) {
        if (specified_u) {
            if (tacc_u64_ule(u64, target->ulong->max)) {
                val->type_kind = TYK_ULONG;
                return val;
            }
            val->type_kind = TYK_ULONGLONG;
        } else if (tacc_u64_ule(u64, target->slong->max)) {
            val->type_kind = TYK_SLONG;
        } else if (can_be_unsigned && tacc_u64_ule(u64, target->ulong->max)) {
            val->type_kind = TYK_ULONG;
        } else if (tacc_u64_ule(u64, target->sllong->max)) {
            val->type_kind = TYK_SLONGLONG;
        } else {
            val->type_kind = TYK_ULONGLONG;
        }
        return val;
    }

    if (specified_u) {
        if (tacc_u64_ule(u64, target->uint->max)) {
            val->type_kind = TYK_UINT;
        } else if (tacc_u64_ule(u64, target->ulong->max)) {
            val->type_kind = TYK_ULONG;
        } else {
            val->type_kind = TYK_ULONGLONG;
        }
        return val;
    }

    if (tacc_u64_ule(u64, target->sint->max)) {
        val->type_kind = TYK_SINT;
    } else if (can_be_unsigned && tacc_u64_ule(u64, target->uint->max)) {
        val->type_kind = TYK_UINT;
    } else if (tacc_u64_ule(u64, target->slong->max)) {
        val->type_kind = TYK_SLONG;
    } else if (can_be_unsigned && tacc_u64_ule(u64, target->ulong->max)) {
        val->type_kind = TYK_ULONG;
    } else if (tacc_u64_ule(u64, target->sllong->max)) {
        val->type_kind = TYK_SLONGLONG;
    } else {
        val->type_kind = TYK_ULONGLONG;
    }
    return val;
}

static struct tacc_val *tacc_parse_charlit(struct tacc_target *target,
                                           struct pp_tok *tok) {
    struct tacc_u64 *u64;
    struct tacc_val *val;
    char *str;
    int input;

    tacc_assert(tok->str != NULL, "need str to parse charlit");
    str = tacc_dynstring_take_str(tok->str);
    u64 = tacc_u64_new();
    str = str + 1;
    while (*str != '\'') {
        tacc_u64_lsh_n(u64, u64, (int) (target->schar->bit_width));
        input = *str;
        if (*str == '\\') {
            str = str + 1;
            switch (*str) {
            case 'a':
                input = 7;
                break;
            case 'b':
                input = 8;
                break;
            case 'f':
                input = 12;
                break;
            case 'n':
                input = 10;
                break;
            case 'r':
                input = 13;
                break;
            case 't':
                input = 9;
                break;
            case '\'':
            case '"':
            case '\\':
                input = *str;
                break;
            case 'x':
                input = (tacc_hex_to_dec(*str)) << 4;
                str = str + 1;
                input = input | tacc_hex_to_dec(*str);
                break;
            default:
                input = *str - '0';
                str = str + 1;
                if ((*str >= '0') && (*str <= '7')) {
                    input = (input << 3) | (*str - '0');
                    str = str + 1;
                    if ((*str >= '0') && (*str <= '7')) {
                        input = (input << 3) | (*str - '0');
                    } else {
                        str = str - 1;
                    }
                } else {
                    str = str - 1;
                }
                break;
            }
        }
        str = str + 1;
        u64->low = u64->low | ((uint32_t) input);
    }

    val = tacc_val_new();
    val->value.int_value = u64;
    val->type_kind = TYK_SINT;

    return val;
}

static void tacc_expr_parse(struct tacc_tok_iter *iter,
                            struct tacc_expr *in_expr);
static void tacc_expr_parse_postfix(struct tacc_tok_iter *iter,
                                    struct tacc_expr *in_expr) {
    struct tacc_expr *expr;
    struct pp_tok *tok;
    struct tacc_expr_list *expr_list;

    expr = in_expr;

    if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
        tok = tacc_tok_iter_peek(iter);
        if (tacc_tok_gives_typename(tok)) {
            expr->kind = EX_COMPOUND_LIT;
            /* TODO */
            tacc_assert(0, "todo compound literals");
        }
        tacc_expr_parse(iter, expr);
        tacc_parse_assert(iter,
                          tacc_tok_iter_accept_tok(iter, TOK_RPAREN),
                          "unmatched lparen");
    } else {
        tok = tacc_tok_iter_peek(iter);
        if (tacc_tok_non_kw_ident(tok)) {
            expr->kind = EX_IDENT;
            tacc_assert(tok->str != NULL, "need str to parse ident");
            expr->extra.name = tacc_dynstring_clone(tok->str);
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else if (tok->kind == TOK_PPNUM) {
            expr->kind = EX_NUM_LIT;
            expr->extra.const_val = tacc_parse_numlit(iter->state->target, tok);
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else if (tok->kind == TOK_STRING) {
            expr->kind = EX_STRING_LIT;
            tacc_parse_error(iter, "todo: string literals");
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else if (tok->kind == TOK_CHAR) {
            expr->kind = EX_NUM_LIT;
            expr->extra.const_val =
                tacc_parse_charlit(iter->state->target, tok);
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else {
            tacc_parse_error(
                iter, "bad expression: %s", tacc_pp_to_string(tok));
        }
    }

    while (1) {
        if (tacc_tok_iter_accept_tok(iter, TOK_LBRACE)) {
            tacc_expr_bump_to_op1(expr);
            expr->op2 = tacc_expr_new();
            expr->kind = EX_SUBSCRIPT;
            tacc_expr_parse(iter, expr->op2);

            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_RBRACE),
                              "unmatched lbrace");
        } else if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
            tacc_expr_bump_to_op1(expr);
            expr->kind = EX_CALL;

            expr_list = tacc_expr_list_new();
            tacc_parse_error(iter, "todo: calls");
            expr->extra.op_list = expr_list;

            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_RBRACE),
                              "unmatched lbrace");
        } else if (tacc_tok_iter_accept_tok(iter, TOK_DOT)) {
            tok = tacc_tok_iter_next(iter);
            tacc_parse_assert(
                iter, tok->kind == TOK_IDENT, "expected member name");
            tacc_expr_bump_to_op1(expr);
            expr->kind = EX_MEMBER;
            tacc_assert(tok->str != NULL, "need str to parse dot");
            expr->extra.name = tacc_dynstring_clone(tok->str);
            tacc_pp_tok_free(tok);
            tok = NULL;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_ARROW)) {
            tok = tacc_tok_iter_next(iter);
            tacc_expr_bump_to_op1(expr);
            expr->kind = EX_PTR_MEMBER;
            tacc_parse_assert(
                iter, tok->kind == TOK_IDENT, "expected member name");
            tacc_assert(tok->str != NULL, "need str to parse arrow");
            expr->extra.name = tacc_dynstring_clone(tok->str);
            tacc_pp_tok_free(tok);
            tok = NULL;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_PLUS_2)) {
            tacc_expr_bump_to_op1(expr);
            expr->kind = EX_INCR_POST;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_MINUS_2)) {
            tacc_expr_bump_to_op1(expr);
            expr->kind = EX_DECR_POST;
        } else {
            break;
        }
    }
}

static struct tacc_expr *tacc_expr_parse_unary(struct tacc_tok_iter *iter,
                                               struct tacc_expr *in_expr) {
    struct tacc_expr *expr;
    struct tacc_expr *next_expr;
    struct pp_tok *tok;

    expr = in_expr;

    while (1) {
        if (tacc_tok_iter_accept_tok(iter, TOK_PLUS_2)) {
            expr->kind = EX_INCR_PRE;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_MINUS_2)) {
            expr->kind = EX_DECR_PRE;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_AMPERSAND)) {
            expr->kind = EX_ADDROF;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_ASTERISK)) {
            expr->kind = EX_DEREF;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_PLUS)) {
            expr->kind = EX_POS;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_MINUS)) {
            expr->kind = EX_NEG;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_TILDE)) {
            expr->kind = EX_BNOT;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_EXCLAMATION)) {
            expr->kind = EX_NOT;
        } else if (tacc_tok_iter_accept_kw(iter, ID_SIZEOF)) {
            expr->kind = EX_SIZEOF;
            if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
                tok = tacc_tok_iter_peek(iter);
                if (tacc_tok_gives_typename(tok)) {
                    expr->kind = EX_SIZEOF_TY;
                    expr->extra.type = tacc_type_parse(iter);
                    return expr;
                } else {
                    tacc_tok_iter_deaccept_tok(iter, TOK_LPAREN);
                }
            }
        } else {
            tacc_expr_parse_postfix(iter, expr);
            return expr;
        }

        next_expr = tacc_expr_new();
        expr->op1 = next_expr;
        expr = next_expr;
    }

    return expr;
}

static void tacc_expr_parse_cast(struct tacc_tok_iter *iter,
                                 struct tacc_expr *in_expr) {
    struct pp_tok *tok;
    struct tacc_type *ty;
    struct tacc_expr *expr;

    expr = in_expr;

    if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
        tok = tacc_tok_iter_peek(iter);
        if (tacc_tok_gives_typename(tok)) {
            ty = tacc_type_parse(iter);
            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_RPAREN),
                              "expected ) to close (");
            tok = tacc_tok_iter_peek(iter);
            if (tok->kind == TOK_RBRACKET) {
                tacc_parse_assert(iter, 0, "todo: compound literals");
            }
            expr->kind = EX_CAST;
            expr->extra.type = ty;
            expr->op1 = tacc_expr_new();
            expr = expr->op1;

            tacc_expr_parse_cast(iter, expr);

            return;
        } else {
            tacc_tok_iter_deaccept_tok(iter, TOK_LPAREN);
        }
    }
    tacc_expr_parse_unary(iter, expr);
}

static tacc_bool tacc_tok_is_assigning(struct pp_tok *tok) {
    switch (tok->kind) {
    case TOK_MINUS_EQ:
    case TOK_AMPERSAND_EQ:
    case TOK_ASTERISK_EQ:
    case TOK_PLUS_EQ:
    case TOK_CIRCUMFLEX_EQ:
    case TOK_PIPE_EQ:
    case TOK_SLASH_EQ:
    case TOK_PERCENT_EQ:
    case TOK_LT_2_EQ:
    case TOK_GT_2_EQ:
    case TOK_EQ:
        return 1;
    default:
        return 0;
    }
}

static enum tacc_expr_kind tacc_tok_to_op(struct pp_tok *tok) {
    switch (tok->kind) {
    case TOK_MINUS_EQ:
        return EX_SUB_ASSI;
    case TOK_AMPERSAND_EQ:
        return EX_BAND_ASSI;
    case TOK_ASTERISK_EQ:
        return EX_MUL_ASSI;
    case TOK_PLUS_EQ:
        return EX_ADD_ASSI;
    case TOK_CIRCUMFLEX_EQ:
        return EX_BXOR_ASSI;
    case TOK_PIPE_EQ:
        return EX_BOR_ASSI;
    case TOK_SLASH_EQ:
        return EX_DIV_ASSI;
    case TOK_PERCENT_EQ:
        return EX_REM_ASSI;
    case TOK_LT_2_EQ:
        return EX_LSH_ASSI;
    case TOK_GT_2_EQ:
        return EX_RSH_ASSI;
    case TOK_EQ:
        return EX_ASSI;
    case TOK_MINUS:
        return EX_SUB;
    case TOK_AMPERSAND:
        return EX_BAND;
    case TOK_AMPERSAND_2:
        return EX_AND;
    case TOK_ASTERISK:
        return EX_MUL;
    case TOK_PLUS:
        return EX_ADD;
    case TOK_EXCLAMATION_EQ:
        return EX_NE;
    case TOK_CIRCUMFLEX:
        return EX_BXOR;
    case TOK_PIPE:
        return EX_BOR;
    case TOK_PIPE_2:
        return EX_OR;
    case TOK_SLASH:
        return EX_DIV;
    case TOK_PERCENT:
        return EX_REM;
    case TOK_LT:
        return EX_LT;
    case TOK_LT_EQ:
        return EX_LE;
    case TOK_LT_2:
        return EX_SHL;
    case TOK_GT:
        return EX_GT;
    case TOK_GT_EQ:
        return EX_GE;
    case TOK_GT_2:
        return EX_SHR;
    case TOK_EQ_2:
        return EX_EQ;
    default:
        tacc_assert(
            0, "unknown operator to convert to op: %s", tacc_pp_to_string(tok));
        return 0;
    }
}

enum tacc_expr_priority {
    PRIO_INVALID,
    PRIO_LOGICAL_OR,
    PRIO_LOGICAL_AND,
    PRIO_INCLUSIVE_OR,
    PRIO_EXCLUSIVE_OR,
    PRIO_AND,
    PRIO_EQUALITY,
    PRIO_RELATIONAL,
    PRIO_SHIFT,
    PRIO_ADDITIVE,
    PRIO_MULTIPLICATIVE,
    PRIO_CAST
};

static enum tacc_expr_priority tacc_tok_to_prio(struct pp_tok *tok) {
    switch (tok->kind) {
    case TOK_MINUS:
        return PRIO_ADDITIVE;
    case TOK_AMPERSAND:
        return PRIO_AND;
    case TOK_AMPERSAND_2:
        return PRIO_LOGICAL_AND;
    case TOK_ASTERISK:
        return PRIO_MULTIPLICATIVE;
    case TOK_PLUS:
        return PRIO_ADDITIVE;
    case TOK_EXCLAMATION_EQ:
        return PRIO_EQUALITY;
    case TOK_CIRCUMFLEX:
        return PRIO_EXCLUSIVE_OR;
    case TOK_PIPE:
        return PRIO_INCLUSIVE_OR;
    case TOK_PIPE_2:
        return PRIO_LOGICAL_OR;
    case TOK_SLASH:
        return PRIO_MULTIPLICATIVE;
    case TOK_PERCENT:
        return PRIO_MULTIPLICATIVE;
    case TOK_LT:
        return PRIO_RELATIONAL;
    case TOK_LT_EQ:
        return PRIO_RELATIONAL;
    case TOK_LT_2:
        return PRIO_SHIFT;
    case TOK_GT:
        return PRIO_RELATIONAL;
    case TOK_GT_EQ:
        return PRIO_RELATIONAL;
    case TOK_GT_2:
        return PRIO_SHIFT;
    case TOK_EQ_2:
        return PRIO_EQUALITY;
    default:
        return PRIO_INVALID;
    }
}

static void tacc_expr_parse_binary(struct tacc_tok_iter *iter,
                                   struct tacc_expr *in_expr,
                                   enum tacc_expr_priority in_prio) {
    struct pp_tok *tok;
    struct tacc_expr *expr;
    enum tacc_expr_priority next_op_prio;

    expr = in_expr;
    while (1) {
        tok = tacc_tok_iter_peek(iter);
        if (tok->kind == TOK_EOF) {
            /* done */
            break;
        }
        next_op_prio = tacc_tok_to_prio(tok);
        if (next_op_prio == PRIO_INVALID) {
            /* not a binary operator */
            break;
        }
        if (next_op_prio < in_prio) {
            /* can only be part of an outer expression */
            break;
        }

        /* accepted the binary operator */
        tok = tacc_tok_iter_next(iter);
        tacc_expr_bump_to_op1(expr);
        expr->kind = tacc_tok_to_op(tok);
        tacc_pp_tok_free(tok);
        tok = NULL;

        expr->op2 = tacc_expr_new();

        /*
         * Binary operator must be followed by a cast-level expression.
         * Mind you, the subexpression here might not be the final subexpression
         * after lower tacc_expr_parse_binary.
         */
        tacc_expr_parse_cast(iter, expr->op2);
        tacc_expr_parse_binary(iter, expr->op2, next_op_prio);

        /*
         * Keep outer expression in expr; it might be the op1
         * of the next expression.
         */
    }
}

static void tacc_expr_parse_conditional(struct tacc_tok_iter *iter,
                                        struct tacc_expr *in_expr) {
    struct tacc_expr *expr;

    expr = in_expr;
    while (1) {
        tacc_expr_parse_binary(iter, expr, PRIO_LOGICAL_OR);
        if (!tacc_tok_iter_accept_tok(iter, TOK_QUESTION)) {
            break;
        }
        tacc_expr_bump_to_op1(expr);
        expr->kind = EX_SELECT;
        expr->op2 = tacc_expr_new();
        expr->op3 = tacc_expr_new();
        tacc_expr_parse(iter, expr->op2);
        expr = expr->op3;
    }
}

static void tacc_expr_parse(struct tacc_tok_iter *iter,
                            struct tacc_expr *in_expr) {
    struct tacc_expr *expr;
    struct pp_tok *tok;

    expr = in_expr;

    while (1) {
        tacc_expr_parse_cast(iter, expr);
        tok = tacc_tok_iter_peek(iter);
        while (tacc_tok_is_assigning(tok)) {
            tacc_assert(expr->kind != EX_CAST, "can't assign to non-lvalue");
            tok = tacc_tok_iter_next(iter);
            tacc_expr_bump_to_op1(expr);
            expr->kind = tacc_tok_to_op(tok);
            tacc_pp_tok_free(tok);
            tok = NULL;

            expr->op2 = tacc_expr_new();
            expr = expr->op2;
            tacc_expr_parse_cast(iter, expr);

            tok = tacc_tok_iter_peek(iter);
        }
        tacc_expr_parse_conditional(iter, expr);

        if (!tacc_tok_iter_accept_tok(iter, TOK_COMMA)) {
            break;
        }
        tacc_expr_bump_to_op1(expr);
        expr->kind = EX_COMMA;
        expr->op2 = tacc_expr_new();
        expr = expr->op2;
    }
}

struct tacc_expr *tacc_expr_parse_new(struct tacc_tok_iter *iter) {
    struct tacc_expr *to_parse;

    to_parse = tacc_expr_new();
    tacc_expr_parse(iter, to_parse);

    return to_parse;
}

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

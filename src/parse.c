#include "parse.h"
#include "3rdparty/intscan.h"
#include "decl.h"
#include "dynstring.h"
#include "tasku_pp.h"
#include "type.h"
#include <memory.h>
#include <stdarg.h>

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

static void tacc_parse_expr_bump_to_op1(struct tacc_expr *expr) {
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

static struct tacc_val *tacc_parse_numlit(struct tacc_type_registry *registry,
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
        val->type = tacc_type_registry_get_basic_type(registry, TYK_SINT);
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
        tacc_u64_copy(&limit, registry->target->sllong->max);
    } else {
        tacc_u64_copy(&limit, registry->target->ullong->max);
    }
    intscan(iter, base, &limit, u64);

    tacc_file_iter_free(iter);

    if (count_l == 2) {
        if (specified_u) {
            val->type =
                tacc_type_registry_get_basic_type(registry, TYK_ULONGLONG);
            return val;
        }
        if (tacc_u64_ugt(u64, registry->target->sllong->max)) {
            val->type =
                tacc_type_registry_get_basic_type(registry, TYK_ULONGLONG);
            return val;
        }
        val->type = tacc_type_registry_get_basic_type(registry, TYK_SLONGLONG);
        return val;
    }
    if (count_l == 1) {
        if (specified_u) {
            if (tacc_u64_ule(u64, registry->target->ulong->max)) {
                val->type =
                    tacc_type_registry_get_basic_type(registry, TYK_ULONG);
                return val;
            }
            val->type =
                tacc_type_registry_get_basic_type(registry, TYK_ULONGLONG);
        } else if (tacc_u64_ule(u64, registry->target->slong->max)) {
            val->type = tacc_type_registry_get_basic_type(registry, TYK_SLONG);
        } else if (can_be_unsigned &&
                   tacc_u64_ule(u64, registry->target->ulong->max)) {
            val->type = tacc_type_registry_get_basic_type(registry, TYK_ULONG);
        } else if (tacc_u64_ule(u64, registry->target->sllong->max)) {
            val->type =
                tacc_type_registry_get_basic_type(registry, TYK_SLONGLONG);
        } else {
            val->type =
                tacc_type_registry_get_basic_type(registry, TYK_ULONGLONG);
        }
        return val;
    }

    if (specified_u) {
        if (tacc_u64_ule(u64, registry->target->uint->max)) {
            val->type = tacc_type_registry_get_basic_type(registry, TYK_UINT);
        } else if (tacc_u64_ule(u64, registry->target->ulong->max)) {
            val->type = tacc_type_registry_get_basic_type(registry, TYK_ULONG);
        } else {
            val->type =
                tacc_type_registry_get_basic_type(registry, TYK_ULONGLONG);
        }
        return val;
    }

    if (tacc_u64_ule(u64, registry->target->sint->max)) {
        val->type = tacc_type_registry_get_basic_type(registry, TYK_SINT);
    } else if (can_be_unsigned &&
               tacc_u64_ule(u64, registry->target->uint->max)) {
        val->type = tacc_type_registry_get_basic_type(registry, TYK_UINT);
    } else if (tacc_u64_ule(u64, registry->target->slong->max)) {
        val->type = tacc_type_registry_get_basic_type(registry, TYK_SLONG);
    } else if (can_be_unsigned &&
               tacc_u64_ule(u64, registry->target->ulong->max)) {
        val->type = tacc_type_registry_get_basic_type(registry, TYK_ULONG);
    } else if (tacc_u64_ule(u64, registry->target->sllong->max)) {
        val->type = tacc_type_registry_get_basic_type(registry, TYK_SLONGLONG);
    } else {
        val->type = tacc_type_registry_get_basic_type(registry, TYK_ULONGLONG);
    }
    return val;
}

static struct tacc_val *tacc_parse_charlit(struct tacc_type_registry *registry,
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
        tacc_u64_lsh_n(u64, u64, (int) (registry->target->schar->bit_width));
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
    val->type = tacc_type_registry_get_basic_type(registry, TYK_SINT);

    return val;
}

static void tacc_parse_expr(struct tacc_tok_iter *iter,
                            struct tacc_expr *in_expr);
static void tacc_parse_expr_postfix(struct tacc_tok_iter *iter,
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
        tacc_parse_expr(iter, expr);
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
            expr->extra.const_val =
                tacc_parse_numlit(iter->state->registry, tok);
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else if (tok->kind == TOK_STRING) {
            expr->kind = EX_STRING_LIT;
            tacc_parse_error(iter, "todo: string literals");
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else if (tok->kind == TOK_CHAR) {
            expr->kind = EX_NUM_LIT;
            expr->extra.const_val =
                tacc_parse_charlit(iter->state->registry, tok);
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else {
            tacc_parse_error(
                iter, "bad expression: %s", tacc_pp_to_string(tok));
        }
    }

    while (1) {
        if (tacc_tok_iter_accept_tok(iter, TOK_LBRACE)) {
            tacc_parse_expr_bump_to_op1(expr);
            expr->op2 = tacc_expr_new();
            expr->kind = EX_SUBSCRIPT;
            tacc_parse_expr(iter, expr->op2);

            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_RBRACE),
                              "unmatched lbrace");
        } else if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
            tacc_parse_expr_bump_to_op1(expr);
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
            tacc_parse_expr_bump_to_op1(expr);
            expr->kind = EX_MEMBER;
            tacc_assert(tok->str != NULL, "need str to parse dot");
            expr->extra.name = tacc_dynstring_clone(tok->str);
            tacc_pp_tok_free(tok);
            tok = NULL;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_ARROW)) {
            tok = tacc_tok_iter_next(iter);
            tacc_parse_expr_bump_to_op1(expr);
            expr->kind = EX_PTR_MEMBER;
            tacc_parse_assert(
                iter, tok->kind == TOK_IDENT, "expected member name");
            tacc_assert(tok->str != NULL, "need str to parse arrow");
            expr->extra.name = tacc_dynstring_clone(tok->str);
            tacc_pp_tok_free(tok);
            tok = NULL;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_PLUS_2)) {
            tacc_parse_expr_bump_to_op1(expr);
            expr->kind = EX_INCR_POST;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_MINUS_2)) {
            tacc_parse_expr_bump_to_op1(expr);
            expr->kind = EX_DECR_POST;
        } else {
            break;
        }
    }
}

static struct tacc_expr *tacc_parse_expr_unary(struct tacc_tok_iter *iter,
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
            tacc_parse_expr_postfix(iter, expr);
            return expr;
        }

        next_expr = tacc_expr_new();
        expr->op1 = next_expr;
        expr = next_expr;
    }

    return expr;
}

static void tacc_parse_expr_cast(struct tacc_tok_iter *iter,
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

            tacc_parse_expr_cast(iter, expr);

            return;
        } else {
            tacc_tok_iter_deaccept_tok(iter, TOK_LPAREN);
        }
    }
    tacc_parse_expr_unary(iter, expr);
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

static void tacc_parse_expr_binary(struct tacc_tok_iter *iter,
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
        tacc_parse_expr_bump_to_op1(expr);
        expr->kind = tacc_tok_to_op(tok);
        tacc_pp_tok_free(tok);
        tok = NULL;

        expr->op2 = tacc_expr_new();

        /*
         * Binary operator must be followed by a cast-level expression.
         * Mind you, the subexpression here might not be the final subexpression
         * after lower tacc_parse_expr_binary.
         */
        tacc_parse_expr_cast(iter, expr->op2);
        tacc_parse_expr_binary(iter, expr->op2, next_op_prio);

        /*
         * Keep outer expression in expr; it might be the op1
         * of the next expression.
         */
    }
}

static void tacc_parse_expr_conditional(struct tacc_tok_iter *iter,
                                        struct tacc_expr *in_expr) {
    struct tacc_expr *expr;

    expr = in_expr;
    while (1) {
        tacc_parse_expr_binary(iter, expr, PRIO_LOGICAL_OR);
        if (!tacc_tok_iter_accept_tok(iter, TOK_QUESTION)) {
            break;
        }
        tacc_parse_expr_bump_to_op1(expr);
        expr->kind = EX_SELECT;
        expr->op2 = tacc_expr_new();
        expr->op3 = tacc_expr_new();
        tacc_parse_expr(iter, expr->op2);
        expr = expr->op3;
    }
}

static void tacc_parse_expr(struct tacc_tok_iter *iter,
                            struct tacc_expr *in_expr) {
    struct tacc_expr *expr;
    struct pp_tok *tok;

    expr = in_expr;

    while (1) {
        tacc_parse_expr_cast(iter, expr);
        tok = tacc_tok_iter_peek(iter);
        while (tacc_tok_is_assigning(tok)) {
            tacc_assert(expr->kind != EX_CAST, "can't assign to non-lvalue");
            tok = tacc_tok_iter_next(iter);
            tacc_parse_expr_bump_to_op1(expr);
            expr->kind = tacc_tok_to_op(tok);
            tacc_pp_tok_free(tok);
            tok = NULL;

            expr->op2 = tacc_expr_new();
            expr = expr->op2;
            tacc_parse_expr_cast(iter, expr);

            tok = tacc_tok_iter_peek(iter);
        }
        tacc_parse_expr_conditional(iter, expr);

        if (!tacc_tok_iter_accept_tok(iter, TOK_COMMA)) {
            break;
        }
        tacc_parse_expr_bump_to_op1(expr);
        expr->kind = EX_COMMA;
        expr->op2 = tacc_expr_new();
        expr = expr->op2;
    }
}

struct tacc_expr *tacc_parse_new_expr(struct tacc_tok_iter *iter) {
    struct tacc_expr *to_parse;

    to_parse = tacc_expr_new();
    tacc_parse_expr(iter, to_parse);

    return to_parse;
}

struct tacc_ast *tacc_ast_new(void) {
    struct tacc_ast *ast = tacc_malloc(sizeof(struct tacc_ast));

    ast->declarations = tacc_decl_list_new();

    return ast;
}

tacc_bool tacc_tok_is_decl_specifier(struct pp_tok *tok) {
    if (tok->kind != TOK_IDENT) {
        return 0;
    }
    switch (tok->ident_kind) {
    case ID_AUTO:
    case ID_CHAR:
    case ID_CONST:
    case ID_DOUBLE:
    case ID_ENUM:
    case ID_EXTERN:
    case ID_FLOAT:
    case ID_INLINE:
    case ID_INT:
    case ID_LONG:
    case ID_REGISTER:
    case ID_RESTRICT:
    case ID_SHORT:
    case ID_SIGNED:
    case ID_STATIC:
    case ID_STRUCT:
    case ID_TYPEDEF:
    case ID_UNION:
    case ID_UNSIGNED:
    case ID_VOID:
    case ID_VOLATILE:
    case ID__BOOL:
    case ID__COMPLEX:
    case ID__IMAGINARY:
        return 1;
    default:
        return 0;
    }
}

enum tacc_intermediate_type_state {
    TYPI_UNSPECIFIED = 0,
    TYPI_INT = 0x1U,
    TYPI_SHORT = 0x2U,
    TYPI_LONG = 0x4U,
    TYPI_LONGLONG = 0x8U,
    TYPI_CHAR = 0x10U,
    TYPI_UNSIGNED = 0x20U,
    TYPI_SIGNED = 0x40U,
    TYPI_OTHER = 0x80U,
    TYPI_FLOAT = 0x100U,
    TYPI_DOUBLE = 0x200U,
    TYPI_LONGDOUBLE = 0x200U,
    TYPI__BOOL = 0x400U,
    TYPI_VOID = 0x800U
};

#define TYPI_SIZE 0x1FU
#define TYPI_SIGN 0x60U

static void tacc_parse_skip_qualifiers(struct tacc_tok_iter *iter) {
    while (1) {
        if (!tacc_tok_iter_accept_kw(iter, ID_CONST) &&
            !tacc_tok_iter_accept_kw(iter, ID_RESTRICT) &&
            !tacc_tok_iter_accept_kw(iter, ID_VOLATILE)) {
            break;
        }
    }
}

struct tacc_declarator *tacc_parse_declarator(struct tacc_tok_iter *iter) {
    struct tacc_declarator *declarator;
    struct tacc_declarator *sub;
    size_t indirection_level;
    struct pp_tok *tok;
    tacc_bool had_static;

    declarator = tacc_declarator_new();
    tok = tacc_tok_iter_peek(iter);
    indirection_level = 0;

    while (tacc_tok_iter_accept_tok(iter, TOK_ASTERISK)) {
        indirection_level = indirection_level + 1;
        tacc_parse_skip_qualifiers(iter);
    }

    if (tok->kind == TOK_IDENT) {
        tok = tacc_tok_iter_next(iter);
        declarator->extra.name = tacc_dynstring_clone(tok->str);
        declarator->kind = DECLARATOR_PLAIN;

        tacc_pp_tok_free(tok);
        tok = NULL;
    } else if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
        sub = tacc_parse_declarator(iter);
        tacc_assert(tacc_tok_iter_accept_tok(iter, TOK_RPAREN),
                    "expected ) in declarator");
    }
    while (1) {
        if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
            tacc_assert(0, "TODO: function declarations");
        } else if (tacc_tok_iter_accept_tok(iter, TOK_LBRACE)) {
            sub = declarator;
            declarator = tacc_declarator_new();
            declarator->kind = DECLARATOR_ARRAY;
            declarator->extra.arr_decl = tacc_array_declarator_new();
            declarator->extra.arr_decl->sub_declarator = sub;

            had_static = tacc_tok_iter_accept_kw(iter, ID_STATIC);
            tacc_parse_skip_qualifiers(iter);
            if (!had_static) {
                had_static = tacc_tok_iter_accept_kw(iter, ID_STATIC);
            }
            if (!had_static) {
                if (tacc_tok_iter_accept_tok(iter, TOK_ASTERISK)) {
                    declarator->extra.arr_decl->array_dim_kind =
                        ARRAYDIM_UNSPECIFIED_VLA;
                    tacc_assert(tacc_tok_iter_accept_tok(iter, TOK_RBRACE),
                                "expected ] in declarator");
                    continue;
                }
            }
            if (tacc_tok_iter_accept_tok(iter, TOK_RBRACE)) {
                declarator->extra.arr_decl->array_dim_kind =
                    ARRAYDIM_UNSPECIFIED;
                continue;
            }
            declarator->extra.arr_decl->array_dim_kind = ARRAYDIM_EXPR;
            declarator->extra.arr_decl->dim_expr = tacc_parse_new_expr(iter);
            tacc_assert(tacc_tok_iter_accept_tok(iter, TOK_RBRACE),
                        "expected ] in declarator");
        } else {
            break;
        }
    }
    declarator->indirection_level = indirection_level;

    return declarator;
}

struct tacc_type *
tacc_parse_typestate_to_type(struct tacc_type_registry *registry,
                             enum tacc_intermediate_type_state state) {
    enum tacc_type_kind kind;

    kind = TYK_SINT;

    switch (state & ~(TYPI_OTHER | TYPI_SIGN)) {
    case TYPI_CHAR:
        if (state & TYPI_UNSIGNED) {
            kind = TYK_UCHAR;
        } else if (state & TYPI_SIGNED) {
            kind = TYK_SCHAR;
        } else {
            kind = TYK_CHAR;
        }
        break;
    case TYPI_SHORT:
        kind = TYK_SSHORT;
        break;
    case TYPI_INT:
        kind = TYK_SINT;
        break;
    case TYPI_LONG:
        kind = TYK_SLONG;
        break;
    case TYPI_LONGLONG:
        kind = TYK_SLONGLONG;
        break;
    case TYPI_FLOAT:
        kind = TYK_FLOAT;
        break;
    case TYPI_DOUBLE:
        kind = TYK_DOUBLE;
        break;
    case TYPI_VOID:
        kind = TYK_VOID;
        break;
    case TYPI__BOOL:
        kind = TYK_BOOL;
        break;
    default:
        tacc_assert(0, "invalid type (internal marker %d)", state);
        break;
    }
    if (state & TYPI_UNSIGNED) {
        kind = tacc_type_to_unsigned(kind);
    }

    return tacc_type_registry_get_basic_type(registry, kind);
}

struct tacc_decl *tacc_parse_new_decl(struct tacc_type_registry *registry,
                                      struct tacc_tok_iter *iter) {
    struct tacc_decl *to_parse;
    struct pp_tok *tok;
    enum tacc_storage_class storage_class;
    enum tacc_intermediate_type_state type_state;
    unsigned type_state_helper;

    to_parse = tacc_decl_new();
    to_parse->kind = DECL_DECLARATORS;
    storage_class = STORAGE_UNSPECIFIED;
    type_state = TYPI_UNSPECIFIED;

    tok = tacc_tok_iter_peek(iter);
    do {
        tacc_assert(tacc_tok_is_decl_specifier(tok),
                    "expected declaration specifier");
        switch (tok->ident_kind) {
        case ID__COMPLEX:
        case ID__IMAGINARY:
            tacc_assert(0, "complex numbers are unsupported");
            break;

        case ID_RESTRICT:
        /* carelessly ignore, even in lieu of all const-correctness constraints
         */
        case ID_CONST:
        /* carelessly ignore, even in lieu of 6.7.3p9 */
        case ID_VOLATILE:
        /* carelessly ignore */
        case ID_INLINE:
            break;

        case ID_AUTO:
            tacc_assert(storage_class == STORAGE_UNSPECIFIED,
                        "multiple storage classes given");
            storage_class = STORAGE_AUTO;
            break;
        case ID_REGISTER:
            tacc_assert(storage_class == STORAGE_UNSPECIFIED,
                        "multiple storage classes given");
            storage_class = STORAGE_REGISTER;
            break;
        case ID_EXTERN:
            tacc_assert(storage_class == STORAGE_UNSPECIFIED,
                        "multiple storage classes given");
            storage_class = STORAGE_EXTERN;
            break;
        case ID_STATIC:
            tacc_assert(storage_class == STORAGE_UNSPECIFIED,
                        "multiple storage classes given");
            storage_class = STORAGE_STATIC;
            break;
        case ID_TYPEDEF:
            tacc_assert(storage_class == STORAGE_UNSPECIFIED,
                        "multiple storage classes given");
            storage_class = STORAGE_TYPEDEF;
            break;

        case ID_ENUM:
            /* TODO */
        case ID_STRUCT:
            /* TODO */
        case ID_UNION:
            /* TODO */

        case ID_CHAR:
            type_state = type_state | TYPI_CHAR;
            break;
        case ID_DOUBLE:
            if ((type_state & TYPI_LONG) != 0) {
                type_state = type_state | TYPI_LONGDOUBLE | TYPI_OTHER;
            } else {
                type_state = type_state | TYPI_DOUBLE | TYPI_OTHER;
            }
            break;
        case ID_FLOAT:
            type_state = type_state | TYPI_FLOAT | TYPI_OTHER;
            break;
        case ID_INT:
            type_state = type_state | TYPI_INT;
            break;
        case ID_LONG:
            if (type_state & TYPI_DOUBLE) {
                type_state = type_state & ~((unsigned) TYPI_DOUBLE);
                type_state = type_state | TYPI_LONGDOUBLE;
            } else if ((type_state & TYPI_LONG) != 0) {
                type_state = type_state & ~((unsigned) TYPI_LONG);
                type_state = type_state | TYPI_LONGLONG;
            } else {
                type_state = type_state | TYPI_LONG;
            }
            break;
        case ID_SHORT:
            type_state = type_state | TYPI_SHORT;
            break;
        case ID_SIGNED:
            type_state = type_state | TYPI_SIGNED;
            break;
        case ID_UNSIGNED:
            type_state = type_state | TYPI_UNSIGNED;
            break;

        case ID_VOID:
            type_state = TYPI_OTHER | TYPI_VOID;
            break;
        case ID__BOOL:
            type_state = TYPI_OTHER | TYPI__BOOL;
            break;
        default:
            tacc_assert(0,
                        "unsupported part of declaration: %s",
                        tacc_pp_to_string(tok));
        }
        tacc_pp_tok_free(tacc_tok_iter_next(iter));
        tok = tacc_tok_iter_peek(iter);
    } while (tacc_tok_is_decl_specifier(tok));

    type_state_helper = (type_state & (~(unsigned) TYPI_SIGN));
    /* ensure only one non-sign marker is set */
    tacc_assert((type_state_helper & (type_state_helper - 1)) == 0,
                "conflicting type");
    if (type_state_helper == 0) {
        type_state = type_state | TYPI_INT;
    }
    /*
     * ensure only one sign marker is set, AND sign is only set if integer type
     * is selected
     */
    type_state_helper = type_state & (TYPI_SIGN | TYPI_OTHER);
    tacc_assert((type_state_helper & (type_state_helper - 1)) == 0,
                "conflicting type");
    if (type_state_helper == 0) {
        type_state = type_state | TYPI_SIGNED;
    }

    to_parse->base_type = tacc_parse_typestate_to_type(registry, type_state);
    to_parse->extra.declarators = tacc_declarator_list_new();
    if (!tacc_tok_iter_accept_tok(iter, TOK_SEMICOLON)) {
        while (1) {
            tacc_declarator_list_push(to_parse->extra.declarators,
                                      tacc_parse_declarator(iter));
            if (tacc_tok_iter_accept_tok(iter, TOK_SEMICOLON)) {
                break;
            }
            tacc_assert(tacc_tok_iter_accept_tok(iter, TOK_COMMA),
                        "expected , or ; in declarator list");
        }
    }

    return to_parse;
}

struct tacc_ast *tacc_parse_file(struct tacc_type_registry *registry,
                                 struct tacc_tok_iter *iter) {
    struct tacc_ast *ast = tacc_ast_new();
    struct pp_tok *tok;

    while (1) {
        tok = tacc_tok_iter_peek(iter);
        if (tok->kind == TOK_EOF) {
            break;
        }
        tacc_decl_list_push(ast->declarations,
                            tacc_parse_new_decl(registry, iter));
    }

    return ast;
}

void tacc_ast_free(struct tacc_ast *ast) {
    tacc_decl_list_free(ast->declarations);
    tacc_free(ast->declarations);
    tacc_free(ast);
}

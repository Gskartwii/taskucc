#include "parse.h"
#include "3rdparty/intscan.h"
#include "decl.h"
#include "dynarray.h"
#include "dynstring.h"
#include "tasku_pp.h"
#include "type.h"
#include <memory.h>
#include <stdarg.h>

enum tacc_declaration_context {
    DECL_CONTEXT_TOP_LEVEL,
    DECL_CONTEXT_OLD_STYLE_PARAMS,
    DECL_CONTEXT_IN_BODY
};

MK_DYNARRAY_OVER(tacc_untagged_ident_list,
                 tacc_untagged_ident_list_entry,
                 struct tacc_untagged_ident *,
                 tacc_untagged_ident_list_new,
                 tacc_untagged_ident_list_init,
                 tacc_untagged_ident_list_get,
                 tacc_untagged_ident_list_push,
                 tacc_untagged_ident_list_pop,
                 tacc_untagged_ident_list_len,
                 tacc_untagged_ident_free,
                 tacc_untagged_ident_list_free)

MK_DYNARRAY_OVER(tacc_ident_scope_list,
                 tacc_ident_scope_list_entry,
                 struct tacc_ident_scope *,
                 tacc_ident_scope_list_new,
                 tacc_ident_scope_list_init,
                 tacc_ident_scope_list_get,
                 tacc_ident_scope_list_push,
                 tacc_ident_scope_list_pop,
                 tacc_ident_scope_list_len,
                 tacc_ident_scope_free,
                 tacc_ident_scope_list_free)

static void
tacc_parse_assert(struct tacc_tok_iter *iter, tacc_bool cond, char *msg, ...) {
    va_list va;
    va_start(va, msg);

    if (cond) {
        return;
    }
    if (!iter->file_iter->filename) {
        fprintf(stderr, "in #if %s:\n", iter->file_iter->src);
    } else {
        fprintf(stderr, "in %s:\naround", iter->file_iter->filename);
        tacc_tok_iter_dump_state(stderr, iter);
    }
    fprintf(stderr, "\n");
    vfprintf(stderr, msg, va);
    fprintf(stderr, "\n");
    tacc_assert(0, "parse error");
}

static void tacc_parse_error(struct tacc_tok_iter *iter, char *msg, ...) {
    va_list va;
    va_start(va, msg);

    if (!iter->file_iter->filename) {
        fprintf(stderr, "in #if %s:\n", iter->file_iter->src);
    } else {
        fprintf(stderr, "in %s:\naround", iter->file_iter->filename);
        tacc_tok_iter_dump_state(stderr, iter);
    }
    vfprintf(stderr, msg, va);
    fprintf(stderr, "\n");
    tacc_assert(0, "parse error");
}

static struct tacc_ident_scope *tacc_ident_scope_new(void) {
    struct tacc_ident_scope *scope;

    scope = tacc_malloc(sizeof(struct tacc_ident_scope));
    scope->untagged_idents = tacc_untagged_ident_list_new();
    scope->tagged_types = tacc_compound_type_list_new();

    return scope;
}

static void
tacc_parse_registry_start_scope(struct tacc_parse_registry *registry) {
    struct tacc_ident_scope *scope;

    scope = tacc_ident_scope_new();
    tacc_ident_scope_list_push(registry->scopes, scope);
}

static void
tacc_parse_registry_end_scope(struct tacc_parse_registry *registry) {
    tacc_ident_scope_free(tacc_ident_scope_list_pop(registry->scopes));
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

static struct tacc_val *tacc_parse_numlit(struct tacc_target *target,
                                          struct tacc_type_list *basic_types,
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
        val->type = tacc_get_basic_type(basic_types, TYK_SINT);
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
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
            return val;
        }
        if (tacc_u64_ugt(u64, target->sllong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
            return val;
        }
        val->type = tacc_get_basic_type(basic_types, TYK_SLONGLONG);
        return val;
    }
    if (count_l == 1) {
        if (specified_u) {
            if (tacc_u64_ule(u64, target->ulong->max)) {
                val->type = tacc_get_basic_type(basic_types, TYK_ULONG);
                return val;
            }
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
        } else if (tacc_u64_ule(u64, target->slong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_SLONG);
        } else if (can_be_unsigned && tacc_u64_ule(u64, target->ulong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONG);
        } else if (tacc_u64_ule(u64, target->sllong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_SLONGLONG);
        } else {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
        }
        return val;
    }

    if (specified_u) {
        if (tacc_u64_ule(u64, target->uint->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_UINT);
        } else if (tacc_u64_ule(u64, target->ulong->max)) {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONG);
        } else {
            val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
        }
        return val;
    }

    if (tacc_u64_ule(u64, target->sint->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_SINT);
    } else if (can_be_unsigned && tacc_u64_ule(u64, target->uint->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_UINT);
    } else if (tacc_u64_ule(u64, target->slong->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_SLONG);
    } else if (can_be_unsigned && tacc_u64_ule(u64, target->ulong->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_ULONG);
    } else if (tacc_u64_ule(u64, target->sllong->max)) {
        val->type = tacc_get_basic_type(basic_types, TYK_SLONGLONG);
    } else {
        val->type = tacc_get_basic_type(basic_types, TYK_ULONGLONG);
    }
    return val;
}

static struct tacc_val *tacc_parse_charlit(struct tacc_target *target,
                                           struct tacc_type_list *basic_types,
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
    val->type = tacc_get_basic_type(basic_types, TYK_SINT);

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
            tacc_parse_assert(iter, 0, "todo compound literals");
        }
        tacc_parse_expr(iter, expr);
        tacc_parse_assert(iter,
                          tacc_tok_iter_accept_tok(iter, TOK_RPAREN),
                          "unmatched lparen");
    } else {
        tok = tacc_tok_iter_peek(iter);
        if (tacc_tok_non_kw_ident(tok)) {
            expr->kind = EX_IDENT;
            tacc_parse_assert(
                iter, tok->str != NULL, "need str to parse ident");
            expr->extra.name = tacc_dynstring_clone(tok->str);
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else if (tok->kind == TOK_PPNUM) {
            expr->kind = EX_NUM_LIT;
            expr->extra.const_val =
                tacc_parse_numlit(iter->state->registry->target,
                                  iter->state->registry->basic_types,
                                  tok);
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else if (tok->kind == TOK_STRING) {
            expr->kind = EX_STRING_LIT;
            tacc_parse_error(iter, "todo: string literals");
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
        } else if (tok->kind == TOK_CHAR) {
            expr->kind = EX_NUM_LIT;
            expr->extra.const_val =
                tacc_parse_charlit(iter->state->registry->target,
                                   iter->state->registry->basic_types,
                                   tok);
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
            tacc_parse_assert(iter, tok->str != NULL, "need str to parse dot");
            expr->extra.name = tacc_dynstring_clone(tok->str);
            tacc_pp_tok_free(tok);
            tok = NULL;
        } else if (tacc_tok_iter_accept_tok(iter, TOK_ARROW)) {
            tok = tacc_tok_iter_next(iter);
            tacc_parse_expr_bump_to_op1(expr);
            expr->kind = EX_PTR_MEMBER;
            tacc_parse_assert(
                iter, tok->kind == TOK_IDENT, "expected member name");
            tacc_parse_assert(
                iter, tok->str != NULL, "need str to parse arrow");
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

static enum tacc_expr_kind tacc_tok_to_op(struct pp_tok *tok,
                                          struct tacc_tok_iter *iter) {
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
        tacc_parse_assert(iter,
                          0,
                          "unknown operator to convert to op: %s",
                          tacc_pp_to_string(tok));
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
        expr->kind = tacc_tok_to_op(tok, iter);
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

static void tacc_parse_assignment_expression(struct tacc_tok_iter *iter,
                                             struct tacc_expr *in_expr) {
    struct tacc_expr *expr;
    struct pp_tok *tok;

    expr = in_expr;

    tacc_parse_expr_cast(iter, expr);
    tok = tacc_tok_iter_peek(iter);
    while (tacc_tok_is_assigning(tok)) {
        tacc_parse_assert(
            iter, expr->kind != EX_CAST, "can't assign to non-lvalue");
        tok = tacc_tok_iter_next(iter);
        tacc_parse_expr_bump_to_op1(expr);
        expr->kind = tacc_tok_to_op(tok, iter);
        tacc_pp_tok_free(tok);
        tok = NULL;

        expr->op2 = tacc_expr_new();
        expr = expr->op2;
        tacc_parse_expr_cast(iter, expr);

        tok = tacc_tok_iter_peek(iter);
    }
    tacc_parse_expr_conditional(iter, expr);
}

static void tacc_parse_expr(struct tacc_tok_iter *iter,
                            struct tacc_expr *in_expr) {
    struct tacc_expr *expr;
    expr = in_expr;

    while (1) {
        tacc_parse_assignment_expression(iter, expr);
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

struct tacc_expr *
tacc_parse_new_assignment_expression(struct tacc_tok_iter *iter) {
    struct tacc_expr *to_parse;

    to_parse = tacc_expr_new();
    tacc_parse_assignment_expression(iter, to_parse);

    return to_parse;
}

struct tacc_expr *
tacc_parse_new_constant_expression(struct tacc_tok_iter *iter) {
    struct tacc_expr *to_parse;

    to_parse = tacc_expr_new();
    /* grammatically equivalent to constant expression */
    tacc_parse_expr_conditional(iter, to_parse);

    return to_parse;
}

struct tacc_ast *tacc_ast_new(void) {
    struct tacc_ast *ast = tacc_malloc(sizeof(struct tacc_ast));

    ast->declarations = tacc_decl_list_new();

    return ast;
}

static struct tacc_untagged_ident *tacc_parse_registry_lookup_untagged(
    struct tacc_parse_registry *registry, char *name) {
    struct tacc_ident_scope_list_entry *scope_entry;
    struct tacc_untagged_ident_list_entry *ident_entry;
    struct tacc_ident_scope *scope;
    size_t i;
    size_t j;
    size_t scope_len;

    scope_len = tacc_ident_scope_list_len(registry->scopes);
    for (i = 1; i <= scope_len; i = i + 1) {
        scope_entry =
            tacc_ident_scope_list_get(registry->scopes, scope_len - i);
        scope = scope_entry->content;
        for (j = 0; j < tacc_untagged_ident_list_len(scope->untagged_idents);
             j = j + 1) {
            ident_entry =
                tacc_untagged_ident_list_get(scope->untagged_idents, j);
            if (!strcmp(name,
                        tacc_dynstring_as_str(ident_entry->content->name))) {
                return ident_entry->content;
            }
        }
    }
    return NULL;
}

/*static struct tacc_compound_type *tacc_parse_registry_lookup_tagged(
    struct tacc_parse_registry *registry, char *name) {
    struct tacc_ident_scope_list_entry *scope_entry;
    struct tacc_compound_type_list_entry *type_entry;
    struct tacc_ident_scope *scope;
    size_t i;
    size_t j;
    size_t scope_len;

    scope_len = tacc_ident_scope_list_len(registry->scopes);
    for (i = 1; i <= scope_len; i = i + 1) {
        scope_entry =
            tacc_ident_scope_list_get(registry->scopes, scope_len - i);
        scope = scope_entry->content;
        for (j = 0; j < tacc_compound_type_list_len(scope->tagged_types);
             j = j + 1) {
            type_entry = tacc_compound_type_list_get(scope->tagged_types, j);
            if (!strcmp(name,
                        tacc_dynstring_as_str(type_entry->content->name))) {
                return type_entry->content;
            }
        }
    }
    return NULL;
}*/

tacc_bool tacc_tok_is_decl_specifier(struct pp_tok *tok,
                                     struct tacc_parse_registry *registry) {
    struct tacc_untagged_ident *ident_descriptor;

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
    case ID_OTHER:
        ident_descriptor = tacc_parse_registry_lookup_untagged(
            registry, tacc_dynstring_as_str(tok->str));
        if (ident_descriptor == NULL) {
            return 0;
        }
        return ident_descriptor->kind == UNTAGGED_IDENT_TYPEDEF;
    default:
        return 0;
    }
}

static void tacc_parse_skip_qualifiers(struct tacc_tok_iter *iter) {
    while (1) {
        if (!tacc_tok_iter_accept_kw(iter, ID_CONST) &&
            !tacc_tok_iter_accept_kw(iter, ID_RESTRICT) &&
            !tacc_tok_iter_accept_kw(iter, ID_VOLATILE)) {
            break;
        }
    }
}

static void tacc_parse_enumerator_list(struct tacc_enumerator_list *out_list,
                                       struct tacc_tok_iter *iter) {
    struct pp_tok *tok;
    struct tacc_enumerator *enumerator;

    while (!tacc_tok_iter_accept_tok(iter, TOK_RBRACKET)) {
        tok = tacc_tok_iter_next(iter);
        tacc_parse_assert(iter,
                          tok->kind == TOK_IDENT && tok->ident_kind == ID_OTHER,
                          "expected enumerator name");
        enumerator = tacc_enumerator_new();
        enumerator->name = tacc_dynstring_clone(tok->str);
        tacc_pp_tok_free(tok);
        tok = NULL;

        if (tacc_tok_iter_accept_tok(iter, TOK_EQ)) {
            enumerator->value = tacc_parse_new_constant_expression(iter);
        }
        tacc_enumerator_list_push(out_list, enumerator);

        if (!tacc_tok_iter_accept_tok(iter, TOK_COMMA)) {
            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_RBRACKET),
                              "expected } to close enumerator list");
            break;
        }
    }
}

static struct tacc_decl_type *
tacc_parse_declaration_specifiers(struct tacc_tok_iter *iter,
                                  enum tacc_storage_class *storage_class_out,
                                  struct tacc_parse_registry *registry);

struct tacc_declarator *
tacc_parse_declarator(struct tacc_tok_iter *iter,
                      struct tacc_parse_registry *registry,
                      tacc_bool allow_abstract);

static void tacc_parse_registry_add_variable(
    struct tacc_parse_registry *registry, struct tacc_string *name) {
    struct tacc_ident_scope_list_entry *scope_entry;
    struct tacc_untagged_ident *ident_descriptor;
    struct tacc_ident_scope *scope;

    scope_entry = tacc_ident_scope_list_get(
        registry->scopes, tacc_ident_scope_list_len(registry->scopes) - 1);
    scope = scope_entry->content;

    ident_descriptor = tacc_malloc(sizeof(struct tacc_untagged_ident));
    ident_descriptor->kind = UNTAGGED_IDENT_OBJECT;
    ident_descriptor->name = name;
    tacc_untagged_ident_list_push(scope->untagged_idents, ident_descriptor);
}

static void tacc_parse_registry_add_typedef(
    struct tacc_parse_registry *registry, struct tacc_declarator *declarator) {
    struct tacc_ident_scope_list_entry *scope_entry;
    struct tacc_untagged_ident *ident_descriptor;
    struct tacc_ident_scope *scope;

    scope_entry = tacc_ident_scope_list_get(
        registry->scopes, tacc_ident_scope_list_len(registry->scopes) - 1);
    scope = scope_entry->content;

    ident_descriptor = tacc_malloc(sizeof(struct tacc_untagged_ident));
    ident_descriptor->kind = UNTAGGED_IDENT_TYPEDEF;
    ident_descriptor->name =
        tacc_dynstring_clone(tacc_declarator_name(declarator));
    tacc_untagged_ident_list_push(scope->untagged_idents, ident_descriptor);
}

static struct tacc_struct_declarator *tacc_parse_struct_declarator(
    struct tacc_tok_iter *iter, struct tacc_parse_registry *registry) {
    struct tacc_struct_declarator *declarator;

    declarator = tacc_struct_declarator_new();
    if (tacc_tok_iter_accept_tok(iter, TOK_COLON)) {
        declarator->bitfield_size = tacc_parse_new_constant_expression(iter);
        return declarator;
    }

    declarator->underlying = tacc_parse_declarator(iter, registry, 0);
    if (tacc_tok_iter_accept_tok(iter, TOK_COLON)) {
        declarator->bitfield_size = tacc_parse_new_constant_expression(iter);
    }

    return declarator;
}

static void tacc_parse_struct_decl_list(struct tacc_struct_decl_list *out_list,
                                        struct tacc_tok_iter *iter,
                                        struct tacc_parse_registry *registry) {
    struct tacc_struct_decl *field_list;
    struct tacc_struct_declarator *declarator;
    enum tacc_storage_class storage_class;

    while (!tacc_tok_iter_accept_tok(iter, TOK_RBRACKET)) {
        field_list = tacc_struct_decl_new();
        field_list->base_type =
            tacc_parse_declaration_specifiers(iter, &storage_class, registry);
        tacc_parse_assert(
            iter,
            storage_class == STORAGE_UNSPECIFIED,
            "cannot specify storage class for struct/union fields");

        while (1) {
            declarator = tacc_parse_struct_declarator(iter, registry);

            tacc_struct_declarator_list_push(field_list->declarators,
                                             declarator);
            if (tacc_tok_iter_accept_tok(iter, TOK_SEMICOLON)) {
                break;
            }
            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_COMMA),
                              "expected , or ; in declarator list");
        }
        tacc_struct_decl_list_push(out_list, field_list);
    }
}

static void tacc_parse_tagged(enum pp_ident_kind kind,
                              struct tacc_decl_type *out_type,
                              struct tacc_tok_iter *iter,
                              struct tacc_parse_registry *registry) {
    struct pp_tok *tok;

    tok = tacc_tok_iter_peek(iter);

    if (tok->kind == TOK_IDENT && tok->ident_kind == ID_OTHER) {
        tok = tacc_tok_iter_next(iter);
        if (kind == ID_ENUM) {
            out_type->extra.enumerators = NULL;
        } else {
            out_type->extra.struct_fields = NULL;
        }
        out_type->referenced_name = tacc_dynstring_clone(tok->str);
        tacc_pp_tok_free(tok);
        tok = tacc_tok_iter_peek(iter);
    } else {
        out_type->referenced_name = NULL;
    }

    if (tok->kind == TOK_LBRACKET) {
        tacc_pp_tok_free(tacc_tok_iter_next(iter));

        if (kind == ID_ENUM) {
            out_type->extra.enumerators = tacc_enumerator_list_new();
            tacc_parse_enumerator_list(out_type->extra.enumerators, iter);
        } else {
            out_type->extra.struct_fields = tacc_struct_decl_list_new();
            tacc_parse_struct_decl_list(
                out_type->extra.struct_fields, iter, registry);
        }
    } else {
        tacc_parse_assert(iter,
                          out_type->referenced_name != NULL,
                          "anonymous struct without definition");
    }
}

#define ENSURE_ONE(flag)                                               \
    tacc_parse_assert(                                                 \
        iter, (type_flags & flag) == 0, "unexpected: %s\n", tok->str); \
    type_flags = type_flags | flag

static struct tacc_decl_type *
tacc_parse_declaration_specifiers(struct tacc_tok_iter *iter,
                                  enum tacc_storage_class *storage_class_out,
                                  struct tacc_parse_registry *registry) {
    enum tacc_storage_class storage_class;
    uint32_t type_flags;
    struct pp_tok *tok;
    struct tacc_decl_type *out_type;
    tacc_bool tok_handled;

    type_flags = 0;
    storage_class = STORAGE_UNSPECIFIED;

    out_type = tacc_malloc(sizeof(struct tacc_decl_type));
    out_type->referenced_name = NULL;

    tok = tacc_tok_iter_peek(iter);
    do {
        tok_handled = 0;
        tacc_parse_assert(iter,
                          tacc_tok_is_decl_specifier(tok, registry),
                          "expected declaration specifier");
        switch (tok->ident_kind) {
        case ID_AUTO:
            tacc_parse_assert(iter,
                              storage_class == STORAGE_UNSPECIFIED,
                              "multiple storage classes given");
            storage_class = STORAGE_AUTO;
            break;
        case ID_REGISTER:
            tacc_parse_assert(iter,
                              storage_class == STORAGE_UNSPECIFIED,
                              "multiple storage classes given");
            storage_class = STORAGE_REGISTER;
            break;
        case ID_EXTERN:
            tacc_parse_assert(iter,
                              storage_class == STORAGE_UNSPECIFIED,
                              "multiple storage classes given");
            storage_class = STORAGE_EXTERN;
            break;
        case ID_STATIC:
            tacc_parse_assert(iter,
                              storage_class == STORAGE_UNSPECIFIED,
                              "multiple storage classes given");
            storage_class = STORAGE_STATIC;
            break;
        case ID_TYPEDEF:
            tacc_parse_assert(iter,
                              storage_class == STORAGE_UNSPECIFIED,
                              "multiple storage classes given");
            storage_class = STORAGE_TYPEDEF;
            break;

        case ID__COMPLEX:
            ENSURE_ONE(TYPESPEC_COMPLEX);
            break;
        case ID__IMAGINARY:
            ENSURE_ONE(TYPESPEC_IMAGINARY);
            break;

        case ID_RESTRICT:
            ENSURE_ONE(TYPEQUAL_RESTRICT);
            break;
        case ID_CONST:
            ENSURE_ONE(TYPEQUAL_CONST);
            break;
        case ID_VOLATILE:
            ENSURE_ONE(TYPEQUAL_VOLATILE);
            break;
        case ID_INLINE:
            ENSURE_ONE(TYPESPEC_INLINE);
            break;

        case ID_ENUM:
            ENSURE_ONE(TYPESPEC_ENUM);

            tacc_pp_tok_free(tacc_tok_iter_next(iter));
            tacc_parse_tagged(ID_ENUM, out_type, iter, registry);
            tok = tacc_tok_iter_peek(iter);
            tok_handled = 1;
            break;
        case ID_STRUCT:
            ENSURE_ONE(TYPESPEC_STRUCT);

            tacc_pp_tok_free(tacc_tok_iter_next(iter));
            tacc_parse_tagged(ID_STRUCT, out_type, iter, registry);
            tok = tacc_tok_iter_peek(iter);
            tok_handled = 1;
            break;
        case ID_UNION:
            ENSURE_ONE(TYPESPEC_UNION);

            tacc_pp_tok_free(tacc_tok_iter_next(iter));
            tacc_parse_tagged(ID_UNION, out_type, iter, registry);
            tok = tacc_tok_iter_peek(iter);
            tok_handled = 1;
            break;

        case ID_CHAR:
            ENSURE_ONE(TYPESPEC_CHAR);
            break;
        case ID_DOUBLE:
            ENSURE_ONE(TYPESPEC_DOUBLE);
            break;
        case ID_FLOAT:
            ENSURE_ONE(TYPESPEC_FLOAT);
            break;
        case ID_INT:
            ENSURE_ONE(TYPESPEC_INT);
            break;
        case ID_LONG:
            tacc_parse_assert(
                iter, (type_flags & TYPESPEC_LONG_2) == 0, "unexpected: long");
            if (type_flags & TYPESPEC_LONG) {
                type_flags = (type_flags & ~((unsigned) TYPESPEC_LONG)) |
                             TYPESPEC_LONG_2;
            } else {
                type_flags = type_flags | TYPESPEC_LONG;
            }
            break;
        case ID_SHORT:
            ENSURE_ONE(TYPESPEC_SHORT);
            break;
        case ID_SIGNED:
            ENSURE_ONE(TYPESPEC_SIGNED);
            break;
        case ID_UNSIGNED:
            ENSURE_ONE(TYPESPEC_UNSIGNED);
            break;

        case ID_VOID:
            ENSURE_ONE(TYPESPEC_VOID);
            break;
        case ID__BOOL:
            ENSURE_ONE(TYPESPEC_BOOL);
            break;
        case ID_OTHER:
            ENSURE_ONE(TYPESPEC_TYPEDEF);
            out_type->referenced_name = tacc_dynstring_clone(tok->str);
            break;
        default:
            tacc_parse_assert(iter,
                              0,
                              "unsupported part of declaration: %s",
                              tacc_pp_to_string(tok));
        }
        if (!tok_handled) {
            tacc_pp_tok_free(tacc_tok_iter_next(iter));
            tok = tacc_tok_iter_peek(iter);
        }
    } while (tacc_tok_is_decl_specifier(tok, registry));

    out_type->spec_qual_flags = type_flags;
    *storage_class_out = storage_class;

    return out_type;
}
#undef ENSURE_ONE

static void tacc_parse_name_list(struct tacc_tok_iter *iter,
                                 struct tacc_string_list *list) {
    struct pp_tok *tok;

    do {
        tok = tacc_tok_iter_next(iter);
        tacc_parse_assert(iter,
                          tok->kind == TOK_IDENT && tok->ident_kind == ID_OTHER,
                          "expected identifier in function parameter list");
        tacc_string_list_push(list, tacc_dynstring_clone(tok->str));
    } while (tacc_tok_iter_accept_tok(iter, TOK_COMMA));
}

void tacc_parse_func_param_list(struct tacc_function_declarator *decl,
                                struct tacc_tok_iter *iter,
                                struct tacc_parse_registry *registry) {
    struct tacc_function_param *param;
    enum tacc_storage_class storage_class;
    struct tacc_string *param_name;

    /*
     * Start temporary scope for the duration of this function parameter
     * list only. An equivalent scope will be re-filled in the function
     * body, if any.
     */
    tacc_parse_registry_start_scope(registry);

    decl->param_list_kind = FUNCPARAM_LIST;
    decl->param_list.modern_params = tacc_function_param_list_new();

    do {
        if (tacc_tok_iter_accept_tok(iter, TOK_DOT_3)) {
            decl->param_list_kind = FUNCPARAM_LIST_VARARG;
            break;
        }
        param = tacc_function_param_new();
        param->base_type =
            tacc_parse_declaration_specifiers(iter, &storage_class, registry);
        tacc_parse_assert(iter,
                          storage_class == STORAGE_UNSPECIFIED ||
                              storage_class == STORAGE_REGISTER,
                          "invalid storage class for function parameter");
        param->decl = tacc_parse_declarator(iter, registry, 1);

        param_name = tacc_declarator_name(param->decl);
        if (param_name != NULL) {
            tacc_parse_registry_add_variable(registry,
                                             tacc_dynstring_clone(param_name));
        }

        tacc_function_param_list_push(decl->param_list.modern_params, param);
    } while (tacc_tok_iter_accept_tok(iter, TOK_COMMA));

    tacc_parse_registry_end_scope(registry);
}

struct tacc_declarator *
tacc_parse_declarator(struct tacc_tok_iter *iter,
                      struct tacc_parse_registry *registry,
                      tacc_bool allow_abstract) {
    struct tacc_declarator *declarator;
    struct tacc_declarator *sub;
    size_t indirection_level;
    struct pp_tok *tok;
    tacc_bool had_static;

    declarator = tacc_declarator_new();
    indirection_level = 0;

    while (tacc_tok_iter_accept_tok(iter, TOK_ASTERISK)) {
        indirection_level = indirection_level + 1;
        tacc_parse_skip_qualifiers(iter);
    }

    tok = tacc_tok_iter_peek(iter);
    if (tok->kind == TOK_IDENT) {
        tok = tacc_tok_iter_next(iter);
        declarator->extra.name = tacc_dynstring_clone(tok->str);
        declarator->kind = DECLARATOR_PLAIN;

        tacc_pp_tok_free(tok);
        tok = NULL;
    } else if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
        if (allow_abstract && tacc_tok_iter_accept_tok(iter, TOK_RPAREN)) {
            sub = tacc_declarator_new();
            sub->kind = DECLARATOR_ABSTRACT;
            declarator->kind = DECLARATOR_FUNC;
            declarator->extra.func_decl->sub_declarator = sub;
            declarator->extra.func_decl->param_list_kind = FUNCPARAM_EMPTY_LIST;
        } else {
            sub = tacc_parse_declarator(iter, registry, allow_abstract);
            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_RPAREN),
                              "expected ) in declarator");
            declarator->kind = DECLARATOR_SUB;
            declarator->extra.sub_declarator = sub;
        }
    } else {
        tacc_parse_assert(iter, allow_abstract, "expected declarator");
        declarator->kind = DECLARATOR_ABSTRACT;
    }
    while (1) {
        if (tacc_tok_iter_accept_tok(iter, TOK_LPAREN)) {
            sub = declarator;
            declarator = tacc_declarator_new();
            declarator->kind = DECLARATOR_FUNC;
            declarator->extra.func_decl = tacc_function_declarator_new();
            declarator->extra.func_decl->sub_declarator = sub;
            if (tacc_tok_iter_accept_tok(iter, TOK_RPAREN)) {
                declarator->extra.func_decl->param_list_kind =
                    FUNCPARAM_EMPTY_LIST;
                continue;
            }
            if (tacc_tok_iter_accept_kw(iter, ID_VOID)) {
                if (tacc_tok_iter_accept_tok(iter, TOK_RPAREN)) {
                    declarator->extra.func_decl->param_list_kind =
                        FUNCPARAM_VOID;
                    continue;
                }
                tacc_tok_iter_deaccept_kw(iter, ID_VOID);
            }
            tok = tacc_tok_iter_peek(iter);
            if (tok->kind == TOK_IDENT && tok->ident_kind == ID_OTHER) {
                /*
                 * only permissible in function definition, but don't check
                 * this yet
                 */
                declarator->extra.func_decl->param_list_kind =
                    FUNCPARAM_OLD_STYLE_LIST;
                declarator->extra.func_decl->param_list.old_style_params =
                    tacc_string_list_new();
                tacc_parse_name_list(
                    iter,
                    declarator->extra.func_decl->param_list.old_style_params);
                tacc_parse_assert(iter,
                                  tacc_tok_iter_accept_tok(iter, TOK_RPAREN),
                                  "expected ) in function declarator");
                continue;
            }
            tacc_parse_func_param_list(
                declarator->extra.func_decl, iter, registry);
            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_RPAREN),
                              "expected ) in function declarator");
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
                    tacc_parse_assert(
                        iter,
                        tacc_tok_iter_accept_tok(iter, TOK_RBRACE),
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
            declarator->extra.arr_decl->dim_expr =
                tacc_parse_new_assignment_expression(iter);
            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_RBRACE),
                              "expected ] in declarator");
        } else {
            break;
        }
    }
    declarator->indirection_level = indirection_level;

    return declarator;
}

static struct tacc_funcdef *
tacc_parse_func_def(struct tacc_decl *decl,
                    struct tacc_parse_registry *registry,
                    struct tacc_tok_iter *iter) {
#ifndef __M2__
    (void) decl;
    (void) registry;
    (void) iter;
#endif
    tacc_parse_assert(iter, 0, "TODO: function definition body");
    return NULL;
}

static struct tacc_decl *
tacc_parse_new_decl(struct tacc_parse_registry *registry,
                    struct tacc_tok_iter *iter,
                    enum tacc_declaration_context accept_funcdef);

static void
tacc_parse_old_style_param_types(struct tacc_decl *decl,
                                 struct tacc_parse_registry *registry,
                                 struct tacc_tok_iter *iter) {
    do {
        tacc_decl_list_push(
            decl->extra.func_def->old_style_param_list,
            tacc_parse_new_decl(registry, iter, DECL_CONTEXT_OLD_STYLE_PARAMS));
    } while (!tacc_tok_iter_accept_tok(iter, TOK_LBRACKET));
}

static struct tacc_decl *
tacc_parse_new_decl(struct tacc_parse_registry *registry,
                    struct tacc_tok_iter *iter,
                    enum tacc_declaration_context ctx) {
    struct tacc_decl *to_parse;
    struct tacc_declarator *declarator;
    enum tacc_storage_class storage_class;

    to_parse = tacc_decl_new();
    to_parse->base_type =
        tacc_parse_declaration_specifiers(iter, &storage_class, registry);
    to_parse->storage_class = storage_class;
    to_parse->extra.declarators = tacc_declarator_list_new();
    if (!tacc_tok_iter_accept_tok(iter, TOK_SEMICOLON)) {
        while (1) {
            declarator = tacc_parse_declarator(iter, registry, 0);

            if (storage_class != STORAGE_TYPEDEF) {
                tacc_parse_registry_add_variable(
                    registry,
                    tacc_dynstring_clone(tacc_declarator_name(declarator)));
            } else {
                tacc_parse_registry_add_typedef(registry, declarator);
            }

            if (ctx == DECL_CONTEXT_TOP_LEVEL) {
                if (declarator->kind == DECLARATOR_FUNC &&
                    declarator->extra.func_decl->param_list_kind ==
                        FUNCPARAM_OLD_STYLE_LIST) {
                    tacc_parse_old_style_param_types(to_parse, registry, iter);
                    to_parse->kind = DECL_FUNCTION_DEF;
                    to_parse->extra.func_def =
                        tacc_parse_func_def(to_parse, registry, iter);
                    tacc_parse_assert(
                        iter,
                        tacc_tok_iter_accept_tok(iter, TOK_RBRACKET),
                        "expected } to close function definition");
                    break;
                }
                if (tacc_tok_iter_accept_tok(iter, TOK_LBRACKET)) {
                    to_parse->kind = DECL_FUNCTION_DEF;
                    to_parse->extra.func_def =
                        tacc_parse_func_def(to_parse, registry, iter);
                    tacc_parse_assert(
                        iter,
                        tacc_tok_iter_accept_tok(iter, TOK_RBRACKET),
                        "expected } to close function definition");
                    break;
                }
            }
            tacc_declarator_list_push(to_parse->extra.declarators, declarator);
            if (tacc_tok_iter_accept_tok(iter, TOK_SEMICOLON)) {
                break;
            }
            tacc_parse_assert(iter,
                              tacc_tok_iter_accept_tok(iter, TOK_COMMA),
                              "expected , or ; in declarator list");
        }
    }

    return to_parse;
}

struct tacc_ast *tacc_parse_file(struct tacc_parse_registry *registry,
                                 struct tacc_tok_iter *iter) {
    struct tacc_ast *ast = tacc_ast_new();
    struct pp_tok *tok;

    while (1) {
        tok = tacc_tok_iter_peek(iter);
        if (tok->kind == TOK_EOF) {
            break;
        }
        tacc_decl_list_push(
            ast->declarations,
            tacc_parse_new_decl(registry, iter, DECL_CONTEXT_TOP_LEVEL));
    }

    return ast;
}

void tacc_ast_free(struct tacc_ast *ast) {
    tacc_decl_list_free(ast->declarations);
    tacc_free(ast->declarations);
    tacc_free(ast);
}

void tacc_untagged_ident_free(struct tacc_untagged_ident *ident) {
    tacc_dynstring_free(ident->name);
    tacc_free(ident);
}

void tacc_ident_scope_free(struct tacc_ident_scope *scope) {
    tacc_compound_type_list_free(scope->tagged_types);
    tacc_free(scope->tagged_types);
    tacc_untagged_ident_list_free(scope->untagged_idents);
    tacc_free(scope->untagged_idents);
    tacc_free(scope);
}

static struct tacc_type *tacc_mk_basic_type(enum tacc_type_kind kind) {
    struct tacc_type *type;

    type = tacc_type_new();
    type->kind = kind;

    return type;
}

struct tacc_parse_registry *tacc_type_registry_new(struct tacc_target *target) {
    struct tacc_parse_registry *registry;
    struct tacc_ident_scope *scope;

    registry = tacc_malloc(sizeof(struct tacc_parse_registry));
    registry->target = target;
    registry->basic_types = tacc_type_list_new();
    registry->scopes = tacc_ident_scope_list_new();

    scope = tacc_ident_scope_new();
    tacc_ident_scope_list_push(registry->scopes, scope);

    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_CHAR));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_SCHAR));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_UCHAR));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_SSHORT));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_USHORT));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_SINT));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_UINT));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_SLONG));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_ULONG));
    tacc_type_list_push(registry->basic_types,
                        tacc_mk_basic_type(TYK_SLONGLONG));
    tacc_type_list_push(registry->basic_types,
                        tacc_mk_basic_type(TYK_ULONGLONG));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_FLOAT));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_DOUBLE));
    tacc_type_list_push(registry->basic_types,
                        tacc_mk_basic_type(TYK_LONGDOUBLE));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_BOOL));
    tacc_type_list_push(registry->basic_types, tacc_mk_basic_type(TYK_VOID));

    return registry;
}

void tacc_type_registry_free(struct tacc_parse_registry *registry) {
    tacc_type_list_free(registry->basic_types);
    tacc_free(registry->basic_types);
    tacc_ident_scope_list_free(registry->scopes);
    tacc_free(registry->scopes);
    tacc_free(registry);
}

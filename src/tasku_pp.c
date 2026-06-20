#include "tasku_pp.h"
#include "dynstring.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if __M2__
#define TACC_PARAM_NOT_FOUND 0xFFFFFFFF
#else
#define TACC_PARAM_NOT_FOUND SIZE_MAX
#endif

tacc_file_iter_p tacc_file_iter_new(void) {
    return tacc_malloc(sizeof(struct tacc_file_iter));
}

static void tacc_file_iter_init_str(tacc_file_iter_p iter,
                                    char *start,
                                    char *end) {
    iter->is_bol = 1;
    iter->is_ws = 1;
    iter->src = start;
    iter->end = end;
    iter->filename = NULL;
}

void tacc_file_iter_init(tacc_file_iter_p iter, tacc_file_p file) {
    tacc_file_iter_init_str(
        iter, file->src, file->src + file->len);
    iter->filename = file->name;
}

pp_tok_p tacc_pp_tok_new(void) { return tacc_malloc(sizeof(struct pp_tok)); }

void tacc_pp_tok_init(pp_tok_p tok) {
    tok->kind = TOK_UNRECOGNIZED;
    tok->ident_kind = ID_OTHER;
    tok->is_final = 0;
    tok->preceded_by_ws = 0;
    tok->str = tacc_dynstring_new();
    tacc_dynstring_init(tok->str);
}

pp_tok_p tacc_pp_tok_clone(pp_tok_p tok) {
    pp_tok_p new_tok;

    new_tok = tacc_pp_tok_new();

    new_tok->kind = tok->kind;
    new_tok->ident_kind = tok->ident_kind;
    new_tok->is_final = tok->is_final;
    new_tok->preceded_by_ws = tok->preceded_by_ws;
    new_tok->str = tok->str;

    return new_tok;
}

char *tacc_pp_to_string(pp_tok_p tok) {
    switch (tok->kind) {
    case TOK_DIRECTIVE:
        return "TOK_DIRECTIVE";
    case TOK_SHARP:
        return "TOK_SHARP";
    case TOK_SHARP_2:
        return "TOK_SHARP_2";
    case TOK_CHAR:
        return "TOK_CHAR";
    case TOK_STRING:
        return "TOK_STRING";
    case TOK_PPNUM:
        return "TOK_PPNUM";
    case TOK_IDENT:
        return "TOK_IDENT";
    case TOK_EOF:
        return "TOK_EOF";
    case TOK_UNRECOGNIZED:
        return "TOK_UNRECOGNIZED";
    case TOK_LBRACE:
        return "TOK_LBRACE";
    case TOK_RBRACE:
        return "TOK_RBRACE";
    case TOK_LPAREN:
        return "TOK_LPAREN";
    case TOK_RPAREN:
        return "TOK_RPAREN";
    case TOK_LBRACKET:
        return "TOK_LBRACKET";
    case TOK_RBRACKET:
        return "TOK_RBRACKET";
    case TOK_DOT:
        return "TOK_DOT";
    case TOK_DOT_3:
        return "TOK_DOT_3";
    case TOK_MINUS:
        return "TOK_MINUS";
    case TOK_ARROW:
        return "TOK_ARROW";
    case TOK_MINUS_2:
        return "TOK_MINUS_2";
    case TOK_MINUS_EQ:
        return "TOK_MINUS_EQ";
    case TOK_AMPERSAND:
        return "TOK_AMPERSAND";
    case TOK_AMPERSAND_2:
        return "TOK_AMPERSAND_2";
    case TOK_AMPERSAND_EQ:
        return "TOK_AMPERSAND_EQ";
    case TOK_ASTERISK:
        return "TOK_ASTERISK";
    case TOK_ASTERISK_EQ:
        return "TOK_ASTERISK_EQ";
    case TOK_PLUS:
        return "TOK_PLUS";
    case TOK_PLUS_2:
        return "TOK_PLUS_2";
    case TOK_PLUS_EQ:
        return "TOK_PLUS_EQ";
    case TOK_TILDE:
        return "TOK_TILDE";
    case TOK_EXCLAMATION:
        return "TOK_EXCLAMATION";
    case TOK_EXCLAMATION_EQ:
        return "TOK_EXCLAMATION_EQ";
    case TOK_CIRCUMFLEX:
        return "TOK_CIRCUMFLEX";
    case TOK_CIRCUMFLEX_EQ:
        return "TOK_CIRCUMFLEX_EQ";
    case TOK_PIPE:
        return "TOK_PIPE";
    case TOK_PIPE_EQ:
        return "TOK_PIPE_EQ";
    case TOK_PIPE_2:
        return "TOK_PIPE_2";
    case TOK_SLASH:
        return "TOK_SLASH";
    case TOK_SLASH_EQ:
        return "TOK_SLASH_EQ";
    case TOK_PERCENT:
        return "TOK_PERCENT";
    case TOK_PERCENT_EQ:
        return "TOK_PERCENT_EQ";
    case TOK_LT:
        return "TOK_LT";
    case TOK_LT_EQ:
        return "TOK_LT_EQ";
    case TOK_LT_2_EQ:
        return "TOK_LT_2_EQ";
    case TOK_LT_2:
        return "TOK_LT_2";
    case TOK_GT:
        return "TOK_GT";
    case TOK_GT_EQ:
        return "TOK_GT_EQ";
    case TOK_GT_2_EQ:
        return "TOK_GT_2_EQ";
    case TOK_GT_2:
        return "TOK_GT_2";
    case TOK_EQ:
        return "TOK_EQ";
    case TOK_EQ_2:
        return "TOK_EQ_2";
    case TOK_QUESTION:
        return "TOK_QUESTION";
    case TOK_COLON:
        return "TOK_COLON";
    case TOK_SEMICOLON:
        return "TOK_SEMICOLON";
    case TOK_COMMA:
        return "TOK_COMMA";
    case TOK_OTHER:
        return "TOK_OTHER";
    case TOK_INCDIR_ANGLE:
        return "TOK_INCDIR_ANGLE";
    case TOK_INCDIR_STRING:
        return "TOK_INCDIR_STRING";
    case TOK_FAKE_END_OF_MACRO:
        return "TOK_FAKE_END_OF_MACRO";
    case TOK_FAKE_PMARK:
        return "TOK_FAKE_PMARK";
    }
    return "UNRECOGNIZED";
}

static void tacc_file_iter_eat_splices(tacc_file_iter_p iter) {
    char ch;
    ch = *iter->src;

    if (ch == '\\') {
        /* c99 5.1.1.2p2: a physical source file cannot end with a backslash */
        ch = iter->src[1];
        if (ch == '\n') {
            iter->src = iter->src + 1;
            iter->src = iter->src + 1;
        }
    }
}

static char *tacc_file_iter_cur(tacc_file_iter_p iter) {
    tacc_file_iter_eat_splices(iter);
    return iter->src;
}

static tacc_bool tacc_file_is_eof(tacc_file_iter_p iter) {
    if (tacc_file_iter_cur(iter) == iter->end) {
        return 1;
    }
    return 0;
}

static char tacc_file_iter_peek_ch(tacc_file_iter_p iter) {
    char *cur;

    cur = tacc_file_iter_cur(iter);
    tacc_assert(!tacc_file_is_eof(iter), "eof while peeking");
    return *cur;
}

static char tacc_file_iter_consume_ch(tacc_file_iter_p iter) {
    char *cur;
    char ch;

    cur = tacc_file_iter_cur(iter);
    tacc_assert(!tacc_file_is_eof(iter), "unexpected eof");
    ch = *cur;

    if (ch == '\n') {
        iter->is_bol = 1;
    } else {
        iter->is_bol = 0;
    }
    if (ch == '\n' || ch == 9 || ch == ' ') {
        iter->is_ws = 1;
    } else {
        iter->is_ws = 0;
    }

    iter->src = iter->src + 1;
    return ch;
}

static tacc_bool tacc_file_iter_accept_ch(tacc_file_iter_p iter, char accept) {
    if (tacc_file_is_eof(iter)) {
        return 0;
    }
    if (tacc_file_iter_peek_ch(iter) == accept) {
        tacc_file_iter_consume_ch(iter);
        return 1;
    }
    return 0;
}

static void tacc_file_iter_eat_comment(tacc_file_iter_p iter) {
    char ch;

    tacc_assert(tacc_file_iter_accept_ch(iter, '/'),
                "error while scanning comment");
    tacc_assert(tacc_file_iter_accept_ch(iter, '*'),
                "error while scanning comment");

    while (1) {
        tacc_assert(!tacc_file_is_eof(iter), "eof in comment");
        ch = tacc_file_iter_consume_ch(iter);
        if (ch == '*') {
            if (tacc_file_iter_accept_ch(iter, '/')) {
                break;
            }
        }
    }
    /* per standard, replaced by single space */
    iter->is_ws = 1;
}

static void tacc_file_iter_eat_new_comment(tacc_file_iter_p iter) {
    char ch;

    tacc_assert(tacc_file_iter_accept_ch(iter, '/'),
                "error while scanning new comment");
    tacc_assert(tacc_file_iter_accept_ch(iter, '/'),
                "error while scanning new comment");

    ch = tacc_file_iter_peek_ch(iter);
    while (ch != '\n') {
        tacc_file_iter_consume_ch(iter);
        ch = tacc_file_iter_peek_ch(iter);
    }
    /* per standard, replaced by single space */
    iter->is_ws = 1;
}

static void tacc_file_iter_eat_ws_no_newlines(tacc_file_iter_p iter) {
    char ch;
    while (1) {
        if (tacc_file_iter_accept_ch(iter, ' ')) {
            continue;
        }
        if (tacc_file_iter_accept_ch(iter, 9)) {
            continue;
        }
        if ((iter->end - iter->src) >= 2) {
            ch = *iter->src;
            if (ch == '/') {
                ch = iter->src[1];
                if (ch == '*') {
                    tacc_file_iter_eat_comment(iter);
                    continue;
                }
                if (ch == '/') {
                    tacc_file_iter_eat_new_comment(iter);
                    continue;
                }
            }
        }
        break;
    }
}

static void tacc_file_iter_eat_all_ws(tacc_file_iter_p iter) {
    char ch;
    while (1) {
        if (tacc_file_iter_accept_ch(iter, ' ')) {
            continue;
        }
        if (tacc_file_iter_accept_ch(iter, 9)) {
            continue;
        }
        if (tacc_file_iter_accept_ch(iter, 10)) {
            continue;
        }
        if ((iter->end - iter->src) >= 2) {
            ch = *iter->src;
            if (ch == '/') {
                ch = iter->src[1];
                if (ch == '*') {
                    tacc_file_iter_eat_comment(iter);
                    continue;
                }
                if (ch == '/') {
                    tacc_file_iter_eat_new_comment(iter);
                    continue;
                }
            }
        }
        break;
    }
}

static void tacc_file_iter_lex_escape(tacc_file_iter_p iter,
                                      tacc_string_p str) {
    char ch;

    ch = tacc_file_iter_consume_ch(iter);
    switch (ch) {
    case '\\':
    case '"':
    case '\'':
    case '?':
    case 'a':
    case 'b':
    case 'e':
    case 'f':
    case 'n':
    case 'r':
    case 't':
    case 'v':
    case 'x':
        tacc_dynstring_push(str, '\\');
        tacc_dynstring_push(str, ch);
        return;
    default:
        tacc_assert(ch >= '0' && ch <= '7', "invalid escape %x", ch);
        tacc_dynstring_push(str, '\\');
        tacc_dynstring_push(str, ch);

        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '7') {
            tacc_dynstring_push(str, ch);
        } else {
            return;
        }
        tacc_file_iter_consume_ch(iter);
        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '7') {
            tacc_file_iter_consume_ch(iter);
            tacc_dynstring_push(str, ch);
        }
        return;
    }
}

static tacc_bool tacc_file_iter_lex_directive(tacc_file_iter_p iter,
                                              pp_tok_p tok_out) {
    tacc_string_p directive_str;
    tacc_bool was_bol, was_ws;

    was_bol = iter->is_bol;
    tacc_file_iter_eat_ws_no_newlines(iter);
    if (!was_bol && !tacc_file_iter_accept_ch(iter, '\n')) {
        return 0;
    }
    tacc_file_iter_eat_all_ws(iter);
    if (!tacc_file_iter_accept_ch(iter, '#')) {
        return 0;
    }

    directive_str = tacc_dynstring_new();
    tacc_dynstring_init(directive_str);
    tok_out->str = directive_str;

    /* file must end in newline, no need to check for eof */
    was_ws = 0;
    while (!tacc_file_iter_accept_ch(iter, '\n')) {
        if (!was_ws) {
            tacc_file_iter_eat_ws_no_newlines(iter);
            if (iter->is_ws) {
                /*
                 * 5.1.1.2p3 permits replacing any nonempty whitespace sequence
                 * with a single space This simplifies comment handling.
                 */
                tacc_dynstring_push(directive_str, ' ');
                was_ws = 1;
                continue;
            }
        }
        tacc_dynstring_push(directive_str, tacc_file_iter_consume_ch(iter));
        was_ws = 0;
    }

    tok_out->kind = TOK_DIRECTIVE;

    return 1;
}

static pp_tok_p tacc_file_iter_lex_char(tacc_file_iter_p iter,
                                        pp_tok_p tok_out) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    pp_tok_p ret = tok_out;
    char contained;
    tacc_string_p out_str;

    out_str = tacc_dynstring_new();
    tacc_dynstring_init(out_str);

    ret->kind = TOK_CHAR;
    ret->str = out_str;
    tacc_dynstring_push(out_str, '\'');

    tacc_assert(!tacc_file_iter_accept_ch(iter, '\''),
                "empty character literal");
    if (!tacc_file_iter_accept_ch(iter, '\\')) {
        contained = tacc_file_iter_consume_ch(iter);
        tacc_assert(tacc_file_iter_accept_ch(iter, '\''),
                    "overlong character literal");

        tacc_dynstring_push(out_str, contained);
        tacc_dynstring_push(out_str, '\'');
        return ret;
    }
    tacc_file_iter_lex_escape(iter, out_str);
    tacc_assert(tacc_file_iter_accept_ch(iter, '\''),
                "overlong character literal");
    tacc_dynstring_push(out_str, '\'');

    return ret;
}

static pp_tok_p tacc_file_iter_lex_string(tacc_file_iter_p iter,
                                          pp_tok_p tok_out) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    pp_tok_p ret = tok_out;
    tacc_string_p out_str;

    ret->kind = TOK_STRING;

    /* maybe make dynamic later */
    out_str = tacc_dynstring_new();
    tacc_dynstring_init(out_str);

    ret->str = out_str;
    tacc_dynstring_push(out_str, '"');

    while (!tacc_file_iter_accept_ch(iter, '"')) {
        tacc_assert(!tacc_file_iter_accept_ch(iter, '\n'),
                    "newline in string literal");
        if (!tacc_file_iter_accept_ch(iter, '\\')) {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        tacc_file_iter_lex_escape(iter, out_str);
    }
    tacc_dynstring_push(out_str, '"');

    return ret;
}

static pp_tok_p tacc_file_iter_lex_incfile(tacc_file_iter_p iter,
                                           pp_tok_p tok_out,
                                           char first) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    pp_tok_p ret = tok_out;
    tacc_string_p out_str;
    char last;

    if (first == '"') {
        last = '"';
    } else {
        last = '>';
    }

    /* maybe make dynamic later */
    out_str = tacc_dynstring_new();
    tacc_dynstring_init(out_str);

    ret->str = out_str;
    tacc_dynstring_push(out_str, first);

    while (!tacc_file_iter_accept_ch(iter, last)) {
        tacc_assert(!tacc_file_iter_accept_ch(iter, '\n'),
                    "newline in string literal");
        if (!tacc_file_iter_accept_ch(iter, '\\')) {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        tacc_file_iter_lex_escape(iter, out_str);
    }
    tacc_dynstring_push(out_str, last);

    return ret;
}

static pp_tok_p tacc_file_iter_lex_ppnum(tacc_file_iter_p iter,
                                         pp_tok_p tok_out,
                                         char first) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    pp_tok_p ret = tok_out;
    tacc_string_p out_str;
    char ch;

    ret->kind = TOK_PPNUM;

    /* maybe make dynamic later */
    out_str = tacc_dynstring_new();
    tacc_dynstring_init(out_str);
    ret->str = out_str;
    tacc_dynstring_push(out_str, first);

    while (1) {
        if (tacc_file_is_eof(iter)) {
            break;
        }
        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '9') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        if (ch == 'p' || ch == 'P' || ch == 'e' || ch == 'E') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            if (tacc_file_iter_accept_ch(iter, '+')) {
                tacc_dynstring_push(out_str, '+');
                continue;
            }
            if (tacc_file_iter_accept_ch(iter, '-')) {
                tacc_dynstring_push(out_str, '-');
                continue;
            }
            continue;
        }
        if (ch >= 'a' && ch <= 'z') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        if (ch >= 'A' && ch <= 'Z') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        if (ch == '_' || ch == '.') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        break;
    }

    return ret;
}

pp_ident_kind_e tacc_recognize_ident_kind(char *ident) {
    if (!strcmp(ident, "auto")) {
        return ID_AUTO;
    }
    if (!strcmp(ident, "break")) {
        return ID_BREAK;
    }
    if (!strcmp(ident, "case")) {
        return ID_CASE;
    }
    if (!strcmp(ident, "char")) {
        return ID_CHAR;
    }
    if (!strcmp(ident, "const")) {
        return ID_CONST;
    }
    if (!strcmp(ident, "continue")) {
        return ID_CONTINUE;
    }
    if (!strcmp(ident, "default")) {
        return ID_DEFAULT;
    }
    if (!strcmp(ident, "do")) {
        return ID_DO;
    }
    if (!strcmp(ident, "double")) {
        return ID_DOUBLE;
    }
    if (!strcmp(ident, "enum")) {
        return ID_ENUM;
    }
    if (!strcmp(ident, "extern")) {
        return ID_EXTERN;
    }
    if (!strcmp(ident, "float")) {
        return ID_FLOAT;
    }
    if (!strcmp(ident, "for")) {
        return ID_FOR;
    }
    if (!strcmp(ident, "goto")) {
        return ID_GOTO;
    }
    if (!strcmp(ident, "inline")) {
        return ID_INLINE;
    }
    if (!strcmp(ident, "int")) {
        return ID_INT;
    }
    if (!strcmp(ident, "long")) {
        return ID_LONG;
    }
    if (!strcmp(ident, "register")) {
        return ID_REGISTER;
    }
    if (!strcmp(ident, "restrict")) {
        return ID_RESTRICT;
    }
    if (!strcmp(ident, "return")) {
        return ID_RETURN;
    }
    if (!strcmp(ident, "short")) {
        return ID_SHORT;
    }
    if (!strcmp(ident, "signed")) {
        return ID_SIGNED;
    }
    if (!strcmp(ident, "sizeof")) {
        return ID_SIZEOF;
    }
    if (!strcmp(ident, "static")) {
        return ID_STATIC;
    }
    if (!strcmp(ident, "struct")) {
        return ID_STRUCT;
    }
    if (!strcmp(ident, "switch")) {
        return ID_SWITCH;
    }
    if (!strcmp(ident, "typedef")) {
        return ID_TYPEDEF;
    }
    if (!strcmp(ident, "union")) {
        return ID_UNION;
    }
    if (!strcmp(ident, "unsigned")) {
        return ID_UNSIGNED;
    }
    if (!strcmp(ident, "void")) {
        return ID_VOID;
    }
    if (!strcmp(ident, "volatile")) {
        return ID_VOLATILE;
    }
    if (!strcmp(ident, "while")) {
        return ID_WHILE;
    }
    if (!strcmp(ident, "_Bool")) {
        return ID__BOOL;
    }
    if (!strcmp(ident, "_Complex")) {
        return ID__COMPLEX;
    }
    if (!strcmp(ident, "_Imaginary")) {
        return ID__IMAGINARY;
    }
    if (!strcmp(ident, "if")) {
        return ID_IF;
    }
    if (!strcmp(ident, "else")) {
        return ID_ELSE;
    }
    return ID_OTHER;
}

static pp_tok_p tacc_file_iter_lex_ident(tacc_file_iter_p iter,
                                         pp_tok_p tok_out,
                                         char first) {
    pp_tok_p ret = tok_out;
    tacc_string_p out_str;
    char ch;

    ret->kind = TOK_IDENT;

    /* maybe make dynamic later */
    out_str = tacc_dynstring_new();
    tacc_dynstring_init(out_str);
    ret->str = out_str;
    tacc_dynstring_push(out_str, first);

    while (1) {
        if (tacc_file_is_eof(iter)) {
            break;
        }
        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '9') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        if (ch >= 'a' && ch <= 'z') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        if (ch >= 'A' && ch <= 'Z') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        if (ch == '_') {
            tacc_dynstring_push(out_str, tacc_file_iter_consume_ch(iter));
            continue;
        }
        break;
    }

    ret->ident_kind =
        tacc_recognize_ident_kind(tacc_dynstring_as_str(out_str));

    return ret;
}

static void tacc_pp_tok_assign_str(pp_tok_p tok, char *str) {
    if (tok->str == NULL) {
        tok->str = tacc_dynstring_new();
        tacc_dynstring_init(tok->str);
    }
    tacc_dynstring_reset(tok->str);
    tacc_dynstring_concat(tok->str, str);
}

static tacc_bool tacc_file_iter_maybe_special(tacc_file_iter_p iter,
                                              pp_tok_p tok_out,
                                              pp_tok_kind_e special_tok,
                                              char *special_match) {
    if (!tacc_file_iter_accept_ch(iter, special_match[1])) {
        tok_out->str = tacc_dynstring_new();
        tacc_dynstring_init(tok_out->str);
        tacc_dynstring_push(tok_out->str, special_match[0]);
        return 0;
    }
    tok_out->kind = special_tok;
    tacc_pp_tok_assign_str(tok_out, special_match);
    return 1;
}

enum tacc_lex_context {
    LEX_TOP_LEVEL,
    LEX_IN_MACRO_ARGS,
    LEX_IN_REPLACEMENT_LIST,
    LEX_IN_INCLUDE,
    LEX_SKIPPING
};
typedef enum tacc_lex_context tacc_lex_context_e;

static pp_tok_p tacc_file_iter_lex(tacc_file_iter_p iter,
                                   tacc_lex_context_e ctx) {
    char first;
    char ch;
    pp_tok_p ret;
    pp_tok_kind_e kind;

    ret = tacc_pp_tok_new();
    tacc_pp_tok_init(ret);

    if (ctx == LEX_SKIPPING) {
        while (1) {
            if (tacc_file_is_eof(iter)) {
                ret->kind = TOK_EOF;
                tacc_pp_tok_assign_str(ret, "");
                return ret;
            }

            ch = tacc_file_iter_peek_ch(iter);
            if (ch != '\n' && ch != '#') {
                tacc_file_iter_consume_ch(iter);
                continue;
            }

            if (tacc_file_iter_lex_directive(iter, ret)) {
                return ret;
            }
        }
    }
    if (tacc_file_is_eof(iter)) {
        ret->kind = TOK_EOF;
        tacc_pp_tok_assign_str(ret, "");
        return ret;
    }

    if (ctx == LEX_TOP_LEVEL) {
        if (tacc_file_iter_lex_directive(iter, ret)) {
            return ret;
        }
    }
    tacc_file_iter_eat_all_ws(iter);

    /*
     * used for correct expansion under # stringification
     * also used for detecting an lparen token invokes a
     * function-like macro
     */
    ret->preceded_by_ws = iter->is_ws;

    if (tacc_file_is_eof(iter)) {
        ret->kind = TOK_EOF;
        tacc_pp_tok_assign_str(ret, "");
        return ret;
    }

    kind = TOK_UNRECOGNIZED;

    first = tacc_file_iter_consume_ch(iter);
    if (!first) {
        return NULL;
    }

    switch (first) {
    case '#':
        tacc_assert(ctx != LEX_TOP_LEVEL, "stray # outside directive");
        kind = TOK_SHARP;
        if (tacc_file_iter_maybe_special(iter, ret, TOK_SHARP_2, "##")) {
            return ret;
        }
        break;
    case '\'':
        return tacc_file_iter_lex_char(iter, ret);
    case '"':
        if (ctx == LEX_IN_INCLUDE) {
            ret->kind = TOK_INCDIR_STRING;
            return tacc_file_iter_lex_incfile(iter, ret, '"');
        }
        return tacc_file_iter_lex_string(iter, ret);
    case '[':
        kind = TOK_LBRACE;
        break;
    case ']':
        kind = TOK_RBRACE;
        break;
    case '(':
        kind = TOK_LPAREN;
        break;
    case ')':
        kind = TOK_RPAREN;
        break;
    case '{':
        kind = TOK_LBRACKET;
        break;
    case '}':
        kind = TOK_RBRACKET;
        break;
    case '.':
        kind = TOK_DOT;
        first = tacc_file_iter_peek_ch(iter);
        if (first >= '0' && first <= '9') {
            return tacc_file_iter_lex_ppnum(iter, ret, '.');
        }
        if (first == '.' &&
            ((iter->end - iter->src) >= 2) &&
            iter->src[1] == '.') {
            ret->kind = TOK_DOT_3;
            tacc_pp_tok_assign_str(ret, "...");
            return ret;
        }
        first = '.';
        break;
    case '-':
        if (tacc_file_iter_maybe_special(iter, ret, TOK_ARROW, "->")) {
            return ret;
        }
        if (tacc_file_iter_maybe_special(iter, ret, TOK_MINUS_2, "--")) {
            return ret;
        }
        ret->kind = TOK_MINUS;
        tacc_file_iter_maybe_special(iter, ret, TOK_MINUS_EQ, "-=");
        return ret;
    case '&':
        if (tacc_file_iter_maybe_special(iter, ret, TOK_AMPERSAND_2, "&&")) {
            return ret;
        }
        ret->kind = TOK_AMPERSAND;
        tacc_file_iter_maybe_special(iter, ret, TOK_AMPERSAND_EQ, "&=");
        return ret;
    case '*':
        ret->kind = TOK_ASTERISK;
        tacc_file_iter_maybe_special(iter, ret, TOK_ASTERISK_EQ, "*=");
        return ret;
    case '+':
        if (tacc_file_iter_maybe_special(iter, ret, TOK_PLUS_2, "++")) {
            return ret;
        }
        ret->kind = TOK_PLUS;
        tacc_file_iter_maybe_special(iter, ret, TOK_PLUS_EQ, "+=");
        return ret;
    case '~':
        kind = TOK_TILDE;
        break;
    case '!':
        ret->kind = TOK_EXCLAMATION;
        tacc_file_iter_maybe_special(iter, ret, TOK_EXCLAMATION_EQ, "!=");
        return ret;
    case '^':
        ret->kind = TOK_CIRCUMFLEX;
        tacc_file_iter_maybe_special(iter, ret, TOK_CIRCUMFLEX_EQ, "^=");
        return ret;
    case '|':
        if (tacc_file_iter_maybe_special(iter, ret, TOK_PIPE_EQ, "|=")) {
            return ret;
        }
        ret->kind = TOK_PIPE;
        tacc_file_iter_maybe_special(iter, ret, TOK_PIPE_2, "||");
        return ret;
    case '/':
        ret->kind = TOK_SLASH;
        tacc_file_iter_maybe_special(iter, ret, TOK_SLASH_EQ, "/=");
        return ret;
    case '%':
        ret->kind = TOK_PERCENT;
        tacc_file_iter_maybe_special(iter, ret, TOK_PERCENT_EQ, "%=");
        return ret;
    case '<':
        if (ctx == LEX_IN_INCLUDE) {
            ret->kind = TOK_INCDIR_ANGLE;
            return tacc_file_iter_lex_incfile(iter, ret, '<');
        }
        ret->kind = TOK_LT;
        if (tacc_file_iter_maybe_special(iter, ret, TOK_LT_EQ, "<=")) {
            return ret;
        }
        if (tacc_file_iter_accept_ch(iter, '<')) {
            if (tacc_file_iter_accept_ch(iter, '=')) {
                ret->kind = TOK_LT_2_EQ;
                tacc_pp_tok_assign_str(ret, "<<=");
                return ret;
            } else {
                ret->kind = TOK_LT_2;
                tacc_pp_tok_assign_str(ret, "<<");
                return ret;
            }
        }
        return ret;
    case '>':
        if (tacc_file_iter_maybe_special(iter, ret, TOK_GT_EQ, ">=")) {
            return ret;
        }
        ret->kind = TOK_GT;
        if (tacc_file_iter_accept_ch(iter, '>')) {
            if (tacc_file_iter_accept_ch(iter, '=')) {
                ret->kind = TOK_GT_2_EQ;
                tacc_pp_tok_assign_str(ret, ">>=");
                return ret;
            } else {
                ret->kind = TOK_GT_2;
                tacc_pp_tok_assign_str(ret, ">>");
                return ret;
            }
        }
        return ret;
    case '=':
        ret->kind = TOK_EQ;
        tacc_file_iter_maybe_special(iter, ret, TOK_EQ_2, "==");
        return ret;
    case '?':
        kind = TOK_QUESTION;
        break;
    case ':':
        kind = TOK_COLON;
        break;
    case ';':
        kind = TOK_SEMICOLON;
        break;
    case ',':
        kind = TOK_COMMA;
        break;
    default:
        if (first >= '0' && first <= '9') {
            return tacc_file_iter_lex_ppnum(iter, ret, first);
        }
        if (first >= 'A' && first <= 'Z') {
            return tacc_file_iter_lex_ident(iter, ret, first);
        }
        if (first >= 'a' && first <= 'z') {
            return tacc_file_iter_lex_ident(iter, ret, first);
        }
        if (first == '_') {
            return tacc_file_iter_lex_ident(iter, ret, first);
        }
        /* Allow the character, for the sake of stringification */
        kind = TOK_OTHER;
        break;
    }

    /* don't include possible following splice */
    tacc_dynstring_push(ret->str, first);
    ret->kind = kind;
    return ret;
}

static pp_tok_p tacc_file_iter_expect_ident(tacc_file_iter_p iter) {
    pp_tok_p tok;
    char ch;

    tok = tacc_pp_tok_new();
    tacc_pp_tok_init(tok);
    ch = tacc_file_iter_consume_ch(iter);
    tacc_assert((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch == '_'),
                "expected identifier, got %x",
                ch);
    tacc_file_iter_lex_ident(iter, tok, ch);

    return tok;
}

tacc_pp_state_p tacc_pp_state_new(void) {
    return tacc_malloc(sizeof(struct tacc_pp_state));
}

tacc_macro_def_entry_p tacc_pp_find_macro_or_first_empty(tacc_pp_state_p state,
                                                         char *name) {
    size_t i;
    tacc_macro_def_entry_p entry;

    entry = state->macros;
    for (i = 0; i < 1024; i = i + 1) {
        if (!entry->content) {
            /* first empty */
            return entry;
        }
        if (!strcmp(
                tacc_dynstring_as_str(
                    entry->content->name),
                name)) {
            /* match, possibly tombstone */
            return entry;
        }
        entry = entry + tacc_sizeadj(1, sizeof(struct tacc_macro_def_entry));
    }
    tacc_assert(0, "too many macros");
    return NULL;
}

void tacc_pp_insert_macro(tacc_pp_state_p state, tacc_macro_def_p macro) {
    tacc_macro_def_entry_p place;

    place = tacc_pp_find_macro_or_first_empty(
        state, tacc_dynstring_as_str(macro->name));
    if (place->content) {
        tacc_assert(
            place->content->is_tombstone,
            "macro defined twice: %s",
            macro->name);
    }
    place->content = macro;
}

void tacc_pp_define(tacc_pp_state_p state, char *name, char *expansion) {
    tacc_token_pp replacement_list;
    tacc_macro_def_p macro;
    tacc_file_iter_p iter;
    pp_tok_p tok;
    size_t replacement_list_len;

    macro = tacc_malloc(sizeof(struct tacc_macro_def));
    macro->name = tacc_dynstring_new();
    tacc_dynstring_init(macro->name);
    tacc_dynstring_concat(macro->name, name);
    macro->is_va = 0;
    macro->is_tombstone = 0;
    macro->num_params = 0;
    macro->is_function_like = 0;
    macro->params = NULL;

    replacement_list = tacc_malloc(sizeof(struct tacc_token_p) * 512);
    macro->replacement_list = replacement_list;
    replacement_list_len = 0;

    iter = tacc_file_iter_new();
    tacc_file_iter_init_str(iter, expansion, expansion + strlen(expansion));

    while (1) {
        tacc_assert(replacement_list_len < 512, "overlong replacement list");
        tok = tacc_file_iter_lex(iter, LEX_IN_REPLACEMENT_LIST);
        if (tok->kind == TOK_EOF) {
            break;
        }
        replacement_list->content = tok;
        replacement_list_len = replacement_list_len + 1;
        replacement_list =
            replacement_list + tacc_sizeadj(1, sizeof(struct tacc_token_p));
    }
    macro->replacement_list_len = replacement_list_len;

    tacc_pp_insert_macro(state, macro);
}

void tacc_pp_undef(tacc_pp_state_p state, char *name) {
    tacc_macro_def_entry_p place;

    place = tacc_pp_find_macro_or_first_empty(state, name);
    if (place->content == NULL) {
        return;
    }
    place->content->is_tombstone = 1;
}

void tacc_pp_state_init(tacc_pp_state_p state) {
    size_t i;
    tacc_include_path_p incpath_entry;
    tacc_macro_def_entry_p macro_entry;

    state->include_path =
        tacc_malloc(sizeof(struct tacc_include_path_entry) * 32);
    state->macros =
        tacc_malloc(sizeof(struct tacc_macro_def_entry) * 1024);

    incpath_entry = state->include_path;
    for (i = 0; i < 32; i = i + 1) {
        incpath_entry->content = NULL;
        incpath_entry = incpath_entry +
                        tacc_sizeadj(1, sizeof(struct tacc_include_path_entry));
    }
    state->include_path->content =
        tacc_dynstring_new();
    tacc_dynstring_init(
        state->include_path->content);
    tacc_dynstring_concat(
        state->include_path->content, ".");
    macro_entry = state->macros;
    for (i = 0; i < 1024; i = i + 1) {
        macro_entry->content = NULL;
        macro_entry =
            macro_entry + tacc_sizeadj(1, sizeof(struct tacc_macro_def_entry));
    }

    tacc_pp_define(state, "__STDC__", "1");
}

tacc_tok_iter_p tacc_tok_iter_new(void) {
    return tacc_malloc(sizeof(struct tacc_tok_iter));
}

void tacc_tok_iter_init(tacc_tok_iter_p iter,
                        tacc_file_iter_p file,
                        tacc_pp_state_p state) {
    iter->file_iter = file;
    iter->pending_len = 0;
    iter->pending =
        tacc_malloc(512 * sizeof(struct tacc_token_p));
    iter->override = NULL;
    iter->state = state;
    iter->inc_level = 0;
    iter->skip_level = 0;
    iter->in_macro_args = 0;
    iter->in_include_directive = 0;
}

static tacc_file_p tacc_pp_try_file(char *base, char *subpath) {
    char *path;
    FILE *file;

    if (strlen(base) + strlen(subpath) >= 254) {
        return NULL;
    }
    path = tacc_malloc(256);
    strcpy(path, base);
    strcat(path, "/");
    strcat(path, subpath);

    file = fopen(path, "r");
    if (!file) {
        return NULL;
    }
    fclose(file);
    return tacc_open(path);
}

static tacc_file_p tacc_pp_search_include_path(tacc_pp_state_p state,
                                               char *cur_file_path,
                                               char *subpath) {
    char *cur_file_dirname;
    char *cur_file_path_cur;
    char *cur_file_path_end;
    tacc_file_p try_file;
    tacc_include_path_p entry;
    size_t i;

    tacc_assert(cur_file_path != NULL, "opened file without path");

    cur_file_path_end = cur_file_path + strlen(cur_file_path);
    cur_file_path_cur = cur_file_path_end;
    cur_file_dirname = NULL;
    while (cur_file_path_cur != cur_file_path) {
        if (*cur_file_path_cur == '/') {
            cur_file_dirname =
                tacc_malloc((size_t) (cur_file_path_cur - cur_file_path) + 1);
            strncpy(cur_file_dirname,
                    cur_file_path,
                    (size_t) (cur_file_path_cur - cur_file_path));
            cur_file_dirname[cur_file_path_cur - cur_file_path] = 0;
            break;
        }
        cur_file_path_cur = cur_file_path_cur - 1;
    }
    if (cur_file_dirname) {
        try_file = tacc_pp_try_file(cur_file_dirname, subpath);
        if (try_file) {
            return try_file;
        }
    }
    entry = state->include_path;
    for (i = 0; i < 32; ++i) {
        if (entry->content == NULL) {
            break;
        }
        try_file = tacc_pp_try_file(
            tacc_dynstring_as_str(entry->content),
            subpath);
        if (try_file) {
            return try_file;
        }
        entry = entry + tacc_sizeadj(1, sizeof(struct tacc_include_path_entry));
    }
    tacc_assert(0, "unable to find suitable file for include: %s", subpath);
    return NULL;
}

static tacc_tok_iter_p tacc_tok_iter_cur_iter(tacc_tok_iter_p first) {
    tacc_tok_iter_p last_iter;

    last_iter = first;

    while (last_iter->override) {
        last_iter = last_iter->override;
    }
    return last_iter;
}

static pp_tok_p tacc_tok_iter_peek_handle_macros(tacc_tok_iter_p);

static void tacc_tok_iter_handle_include(tacc_tok_iter_p first,
                                         tacc_file_iter_p iter) {
    char *hdr_name;
    char *hdr_name_start;
    pp_tok_p tok;
    tacc_tok_iter_p tok_iter;
    tacc_tok_iter_p last_iter;
    tacc_file_p included_file;
    tacc_file_iter_p included_file_iter;
    tacc_tok_iter_p included_file_tok_iter;

    tok_iter = tacc_tok_iter_new();
    tacc_tok_iter_init(tok_iter, iter, first->state);
    tok_iter->in_include_directive = 1;

    tok = tacc_tok_iter_peek_handle_macros(tok_iter);
    if (tok->kind != TOK_INCDIR_ANGLE) {
        tacc_assert(tok->kind == TOK_INCDIR_STRING,
                    "expected include file string, got %s",
                    tacc_pp_to_string(tok));
    }

    hdr_name = tacc_malloc(1024);
    hdr_name_start = hdr_name;

    tacc_assert(tacc_dynstring_len(tok->str) > 2,
                "empty include string");
    tacc_assert(tacc_dynstring_len(tok->str) < 1024,
                "overlong include string");
    strcpy(hdr_name, tacc_dynstring_as_str(tok->str) + 1);
    hdr_name[tacc_dynstring_len(tok->str) - 2] = 0; /* drop > or " */

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #include");

    last_iter = tacc_tok_iter_cur_iter(first);

    included_file = tacc_pp_search_include_path(
        first->state,
        last_iter->file_iter->filename,
        hdr_name_start);
    included_file_iter = tacc_file_iter_new();
    tacc_file_iter_init(included_file_iter, included_file);
    included_file_tok_iter = tacc_tok_iter_new();
    tacc_tok_iter_init(
        included_file_tok_iter, included_file_iter, first->state);
    last_iter->override = included_file_tok_iter;
}

static void tacc_tok_iter_handle_define(tacc_tok_iter_p first,
                                        tacc_file_iter_p iter) {
    pp_tok_p tok;
    tacc_string_p macro_name;
    tacc_macro_def_p macro;
    tacc_ident_p params;
    tacc_token_pp replacement_list;
    size_t i;
    size_t replacement_list_len;
    tacc_bool terminated;

    macro = tacc_malloc(sizeof(struct tacc_macro_def));
    tok = tacc_file_iter_expect_ident(iter);
    macro_name = tok->str;
    macro->name = tacc_dynstring_clone(macro_name);
    macro->is_va = 0;
    macro->is_tombstone = 0;
    macro->is_function_like = 0;

    /* don't eat whitespace before lparen */
    if (tacc_file_iter_accept_ch(iter, '(')) {
        macro->is_function_like = 1;
        tacc_file_iter_eat_ws_no_newlines(iter);
        /* function-like macro */
        params = tacc_malloc(8 * sizeof(struct tacc_ident));
        macro->params = params;
        if (tacc_file_iter_accept_ch(iter, ')')) {
            macro->num_params = 0;
        } else {
            terminated = 0;
            for (i = 0; i < 8; ++i) {
                tacc_file_iter_eat_ws_no_newlines(iter);
                if (tacc_file_iter_accept_ch(iter, '.')) {
                    tacc_assert(tacc_file_iter_accept_ch(iter, '.'),
                                "expected ...");
                    tacc_assert(tacc_file_iter_accept_ch(iter, '.'),
                                "expected ...");
                    tacc_file_iter_eat_ws_no_newlines(iter);
                    tacc_assert(tacc_file_iter_accept_ch(iter, ')'),
                                "expected )");

                    /* has params up to and excluding i */
                    macro->num_params = i;
                    macro->is_va = 1;
                    terminated = 1;
                    break;
                }
                tok = tacc_file_iter_expect_ident(iter);
                params->content = tok->str;
                tacc_file_iter_eat_ws_no_newlines(iter);
                if (tacc_file_iter_accept_ch(iter, ',')) {
                    params = params + tacc_sizeadj(1, sizeof(tacc_ident_p));
                    continue;
                }
                tacc_assert(tacc_file_iter_accept_ch(iter, ')'),
                            "expected , or )");
                terminated = 1;
                macro->num_params = i + 1;
                break;
            }
            tacc_assert(terminated, "overlong macro parameter list");
        }
        tacc_file_iter_eat_ws_no_newlines(iter);
    } else {
        tacc_file_iter_eat_ws_no_newlines(iter);
        macro->num_params = 0;
        macro->params = NULL;
    }
    /* no ws necessarily required here!!! huh??? */

    replacement_list = tacc_malloc(sizeof(struct tacc_token_p) * 512);
    macro->replacement_list = replacement_list;
    replacement_list_len = 0;
    while (1) {
        tacc_assert(replacement_list_len < 512, "overlong replacement list");
        tok = tacc_file_iter_lex(iter, LEX_IN_REPLACEMENT_LIST);
        if (tok->kind == TOK_EOF) {
            break;
        }
        replacement_list->content = tok;
        replacement_list_len = replacement_list_len + 1;
        replacement_list =
            replacement_list + tacc_sizeadj(1, sizeof(struct tacc_token_p));
    }
    macro->replacement_list_len = replacement_list_len;

    tacc_pp_insert_macro(first->state, macro);
}

static void tacc_tok_iter_handle_undef(tacc_tok_iter_p first,
                                       tacc_file_iter_p iter) {
    pp_tok_p tok;

    tok = tacc_file_iter_expect_ident(iter);
    tacc_pp_undef(first->state,
                  tacc_dynstring_as_str(tok->str));
}

static void tacc_tok_iter_handle_ifndef(tacc_tok_iter_p first,
                                        tacc_file_iter_p iter) {
    pp_tok_p tok;
    tacc_tok_iter_p last_iter;
    tacc_macro_def_entry_p macro_def;

    tok = tacc_file_iter_expect_ident(iter);

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #ifndef");

    last_iter = tacc_tok_iter_cur_iter(first);

    macro_def = tacc_pp_find_macro_or_first_empty(
        first->state, tacc_dynstring_as_str(tok->str));
    if (macro_def->content) {
        last_iter->inc_level =
            last_iter->inc_level + 1;
        return;
    }
    last_iter->skip_level =
        last_iter->skip_level + 1;
}

static void tacc_tok_iter_handle_ifdef(tacc_tok_iter_p first,
                                       tacc_file_iter_p iter) {
    pp_tok_p tok;
    tacc_tok_iter_p last_iter;
    tacc_macro_def_entry_p macro_def;

    tok = tacc_file_iter_expect_ident(iter);

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #ifdef");

    last_iter = tacc_tok_iter_cur_iter(first);

    macro_def = tacc_pp_find_macro_or_first_empty(
        first->state, tacc_dynstring_as_str(tok->str));
    if (!macro_def->content) {
        last_iter->inc_level =
            last_iter->inc_level + 1;
        return;
    }
    last_iter->skip_level =
        last_iter->skip_level + 1;
}

static void tacc_tok_iter_handle_endif(tacc_tok_iter_p first,
                                       tacc_file_iter_p iter) {
    tacc_tok_iter_p last_iter;

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #endif");

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_level > 0) {
        last_iter->skip_level =
            last_iter->skip_level - 1;
        return;
    }
    tacc_assert(last_iter->inc_level > 0, "stray #endif");
    last_iter->inc_level = last_iter->inc_level - 1;
}

static void tacc_tok_iter_handle_else(tacc_tok_iter_p first,
                                      tacc_file_iter_p iter) {
    tacc_tok_iter_p last_iter;

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #else");

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_level == 1) {
        last_iter->skip_level = 0;
        last_iter->inc_level =
            last_iter->inc_level + 1;
        return;
    }
    if (last_iter->skip_level > 1) {
        return;
    }
    tacc_assert(last_iter->inc_level > 0, "stray #else");
    last_iter->inc_level = last_iter->inc_level - 1;
    last_iter->skip_level =
        last_iter->skip_level + 1;
}

static void tacc_tok_iter_handle_if(tacc_tok_iter_p first,
                                    tacc_file_iter_p iter) {
    tacc_file_iter_eat_ws_no_newlines(iter);

    /* TODO: conditional inclusion */
#ifdef __STDC__
    (void) first;
#endif
}

static void tacc_tok_iter_handle_error_directive(tacc_tok_iter_p first,
                                                 tacc_file_iter_p iter) {
    tacc_file_iter_eat_ws_no_newlines(iter);

    tacc_assert(0, "#error: %s", iter->src);

#ifdef __STDC__
    (void) first;
#endif
}

static void tacc_tok_iter_handle_directive(tacc_tok_iter_p first,
                                           tacc_string_p directive) {
    tacc_file_iter_p dir_scanner;
    pp_tok_p tok;
    char *directive_name;

    dir_scanner = tacc_file_iter_new();
    tacc_file_iter_init_str(dir_scanner,
                            tacc_dynstring_as_str(directive),
                            tacc_dynstring_as_str(directive) +
                                tacc_dynstring_len(directive));

    tacc_file_iter_eat_ws_no_newlines(dir_scanner);
    if (tacc_file_is_eof(dir_scanner) ||
        tacc_file_iter_accept_ch(dir_scanner, '\n')) {
        /* empty directive, skip */
        return;
    }
    tok = tacc_file_iter_expect_ident(dir_scanner);
    tacc_file_iter_eat_ws_no_newlines(dir_scanner);
    directive_name = tacc_dynstring_as_str(tok->str);

    if (!strcmp(directive_name, "include")) {
        tacc_assert(dir_scanner->is_ws,
                    "expected whitespace after #include");
        tacc_tok_iter_handle_include(first, dir_scanner);
        return;
    }
    if (!strcmp(directive_name, "define")) {
        tacc_assert(dir_scanner->is_ws,
                    "expected whitespace after #define");
        tacc_tok_iter_handle_define(first, dir_scanner);
        return;
    }
    if (!strcmp(directive_name, "undef")) {
        tacc_assert(dir_scanner->is_ws,
                    "expected whitespace after #undef");
        tacc_tok_iter_handle_undef(first, dir_scanner);
        return;
    }
    if (!strcmp(directive_name, "if")) {
        tacc_tok_iter_handle_if(first, dir_scanner);
        return;
    }
    if (!strcmp(directive_name, "ifdef")) {
        tacc_assert(dir_scanner->is_ws,
                    "expected whitespace after #ifdef");
        tacc_tok_iter_handle_ifdef(first, dir_scanner);
        return;
    }
    if (!strcmp(directive_name, "ifndef")) {
        tacc_assert(dir_scanner->is_ws,
                    "expected whitespace after #ifndef");
        tacc_tok_iter_handle_ifndef(first, dir_scanner);
        return;
    }
    if (!strcmp(directive_name, "endif")) {
        tacc_tok_iter_handle_endif(first, dir_scanner);
        return;
    }
    if (!strcmp(directive_name, "else")) {
        tacc_tok_iter_handle_else(first, dir_scanner);
        return;
    }
    if (!strcmp(directive_name, "error")) {
        tacc_tok_iter_handle_error_directive(first, dir_scanner);
        return;
    }
    tacc_assert(0, "unknown directive: %s", tok->str);
}

static pp_tok_p tacc_tok_maybe_finalize(tacc_tok_iter_p iter, pp_tok_p tok) {
    tacc_macro_def_entry_p macro_def_entry;
    tacc_macro_def_p macro_def;
    pp_tok_p new_tok;

    if ((tok->kind != TOK_IDENT) || (tok->is_final)) {
        return tok;
    }
    macro_def_entry = tacc_pp_find_macro_or_first_empty(
        iter->state, tacc_dynstring_as_str(tok->str));
    macro_def = macro_def_entry->content;
    if (!macro_def) {
        return tok;
    }
    if (!macro_def->is_replacing) {
        return tok;
    }

    new_tok = tacc_pp_tok_clone(tok);
    new_tok->is_final = 1;

    return new_tok;
}

static pp_tok_p tacc_tok_iter_peek_nomacro(tacc_tok_iter_p iter) {
    pp_tok_p tok;
    tacc_macro_def_entry_p macro_entry;
    tacc_macro_def_p macro_def;
    tacc_token_pp entry;

    if (iter->pending_len == 0) {
        if (iter->in_macro_args) {
            tok =
                tacc_file_iter_lex(iter->file_iter, LEX_IN_MACRO_ARGS);
        } else if (iter->skip_level > 0) {
            tok = tacc_file_iter_lex(iter->file_iter, LEX_SKIPPING);
        } else if (iter->in_include_directive) {
            tok = tacc_file_iter_lex(iter->file_iter, LEX_IN_INCLUDE);
        } else {
            tok = tacc_file_iter_lex(iter->file_iter, LEX_TOP_LEVEL);
        }
        tok = tacc_tok_maybe_finalize(iter, tok);
        iter->pending->content = tok;
        iter->pending_len = 1;
        return tok;
    }
    entry = iter->pending +
            tacc_sizeadj((iter->pending_len - 1),
                         sizeof(struct tacc_token_p));
    tok = entry->content;
    tok = tacc_tok_maybe_finalize(iter, tok);
    if (tok->kind == TOK_FAKE_END_OF_MACRO) {
        iter->pending_len = iter->pending_len - 1;
        macro_entry = tacc_pp_find_macro_or_first_empty(
            iter->state, tacc_dynstring_as_str(tok->str));
        macro_def = macro_entry->content;
        tacc_assert(macro_def != NULL,
                    "failed to find macrodef for macro being expanded: %s",
                    tok->str);
        macro_def->is_replacing = 0;
        /* simple recursion, this shouldn't go too deep */
        return tacc_tok_iter_peek_nomacro(iter);
    }
    if (tok->kind == TOK_FAKE_PMARK) {
        iter->pending_len = iter->pending_len - 1;
        /* simple recursion, this shouldn't go too deep */
        return tacc_tok_iter_peek_nomacro(iter);
    }
    return tok;
}

static pp_tok_p tacc_tok_iter_consume_nomacro(tacc_tok_iter_p iter) {
    pp_tok_p tok = tacc_tok_iter_peek_nomacro(iter);
    iter->pending_len = iter->pending_len - 1;
    return tok;
}

static void tacc_tok_iter_push_pending(tacc_tok_iter_p iter, pp_tok_p tok) {
    tacc_token_pp tok_entry;

    tok_entry = iter->pending +
                tacc_sizeadj(iter->pending_len,
                             sizeof(struct tacc_token_p));
    tok_entry->content = tok;
    iter->pending_len = iter->pending_len + 1;
}

static void tacc_tok_iter_join_pending(tacc_tok_iter_p iter, pp_tok_p tok) {
    tacc_token_pp tok_entry;
    tacc_file_iter_p file_iter;
    pp_tok_p old_tok;
    pp_tok_p new_tok;
    tacc_string_p new_tok_str;

    tok_entry = iter->pending +
                tacc_sizeadj(iter->pending_len - 1,
                             sizeof(struct tacc_token_p));
    old_tok = tok_entry->content;
    new_tok_str = tacc_dynstring_new();
    tacc_dynstring_init(new_tok_str);

    /*
     * pending is pushed in right-to-left order.
     * Later tokens will already be present in the stack, so we join in reverse
     * order.
     */
    tacc_dynstring_join(new_tok_str, tok->str);
    tacc_dynstring_join(new_tok_str, old_tok->str);

    file_iter = tacc_file_iter_new();
    tacc_file_iter_init_str(file_iter,
                            tacc_dynstring_as_str(new_tok_str),
                            tacc_dynstring_as_str(new_tok_str) +
                                tacc_dynstring_len(new_tok_str));

    new_tok = tacc_file_iter_lex(file_iter, LEX_IN_REPLACEMENT_LIST);
    new_tok->preceded_by_ws = tok->preceded_by_ws;
    tacc_file_iter_eat_ws_no_newlines(file_iter);
    tacc_assert(tacc_file_is_eof(file_iter),
                "multiple tokens produced by joining: %s",
                new_tok_str);

    tok_entry->content = new_tok;
}

static void tacc_tok_iter_insert_macro_replacing_stop(tacc_tok_iter_p iter,
                                                      char *macro_name) {
    pp_tok_p tok;

    tok = tacc_pp_tok_new();
    tacc_pp_tok_init(tok);
    tok->kind = TOK_FAKE_END_OF_MACRO;
    tacc_dynstring_concat(tok->str, macro_name);
    tacc_tok_iter_push_pending(iter, tok);
}

static void tacc_tok_iter_push_placemarker(tacc_tok_iter_p iter) {
    pp_tok_p tok;

    tok = tacc_pp_tok_new();
    tacc_pp_tok_init(tok);
    tok->kind = TOK_FAKE_PMARK;
    tacc_tok_iter_push_pending(iter, tok);
}

static size_t tacc_macro_def_index_of_par(tacc_macro_def_p macro_def,
                                          char *ident) {
    size_t i;
    tacc_ident_p par_ident;

    if (macro_def->is_va && (!strcmp(ident, "__VA_ARGS__"))) {
        return macro_def->num_params;
    }
    for (i = 0; i < macro_def->num_params; ++i) {
        par_ident = macro_def->params +
                    tacc_sizeadj(i, sizeof(struct tacc_ident));
        if (!strcmp(tacc_dynstring_as_str(par_ident->content),
                    ident)) {
            return i;
        }
    }

    return TACC_PARAM_NOT_FOUND;
}

static tacc_token_p_list_p tacc_pp_split_args(tacc_macro_def_p macro_def,
                                              tacc_token_pp raw_args,
                                              size_t raw_args_len) {
    tacc_token_p_list_p ret;
    tacc_token_p_list_p ret_cur;
    pp_tok_p raw_arg;
    tacc_token_pp raw_arg_entry;
    size_t nest_level;
    size_t i;
    size_t start_j;
    tacc_bool had_comma;
    size_t j;
    size_t max_param;

    nest_level = 0;
    tacc_assert(macro_def->is_function_like,
                "object-like macro takes no params");
    /* +1 to leave space for overflow, even if varargs are not used */
    ret = tacc_malloc((macro_def->num_params + 1) *
                      sizeof(struct tacc_token_p_list));
    j = 0;
    max_param = macro_def->num_params;
    if (macro_def->is_va) {
        max_param = max_param + 1;
    }
    for (i = 0; i < max_param; i = i + 1) {
        ret_cur = ret + tacc_sizeadj(i, sizeof(struct tacc_token_p_list));
        if (j >= raw_args_len) {
            ret_cur->list = NULL;
            ret_cur->list_len = 0;
            continue;
        }
        start_j = j;
        had_comma = 0;
        while (j < raw_args_len) {
            raw_arg_entry =
                raw_args + tacc_sizeadj(j, sizeof(struct tacc_token_p));
            raw_arg = raw_arg_entry->content;

            if ((nest_level == 0) && (raw_arg->kind == TOK_COMMA) &&
                (i < macro_def->num_params)) {
                /* if i == tacc_macro_def_num_params, we are looking at va_args
                 */
                had_comma = 1;
                j = j + 1;
                break;
            }
            if (raw_arg->kind == TOK_LPAREN) {
                nest_level = nest_level + 1;
            }
            if (raw_arg->kind == TOK_RPAREN) {
                nest_level = nest_level - 1;
            }
            j = j + 1;
        }
        tacc_assert(nest_level == 0, "missing ) in macro argument list");

        ret_cur->list_len = j - start_j;
        if (had_comma) {
            ret_cur->list_len = ret_cur->list_len - 1;
        }
        /* allocate +1 to avoid 0-size allocations */
        ret_cur->list = tacc_malloc(
            (ret_cur->list_len + 1) * sizeof(struct tacc_token_p));
        memcpy(ret_cur->list,
               raw_args + tacc_sizeadj(start_j, sizeof(struct tacc_token_p)),
               ret_cur->list_len * sizeof(struct tacc_token_p));
    }

    return ret;
}

static pp_tok_p tacc_pp_stringify(tacc_token_pp tokens, size_t num_tokens) {
    pp_tok_p ret;
    tacc_string_p ret_tok_str;
    tacc_string_p this_tok_str;
    tacc_token_pp tok_entry;
    pp_tok_p tok;
    size_t i;
    size_t j;
    char ch;

    ret_tok_str = tacc_dynstring_new();
    tacc_dynstring_init(ret_tok_str);
    ret = tacc_pp_tok_new();
    tacc_pp_tok_init(ret);
    ret->kind = TOK_STRING;
    ret->str = ret_tok_str;

    tok_entry = tokens;
    tacc_dynstring_push(ret_tok_str, '"');
    for (i = 0; i < num_tokens; i = i + 1) {
        tok = tok_entry->content;
        if ((i != 0) && tok->preceded_by_ws) {
            tacc_dynstring_push(ret_tok_str, ' ');
        }
        if ((tok->kind == TOK_STRING) ||
            (tok->kind == TOK_CHAR)) {
            this_tok_str = tok->str;
            for (j = 0; j < (size_t) tacc_dynstring_len(this_tok_str);
                 j = j + 1) {
                ch = tacc_dynstring_at(this_tok_str, j);
                if (ch == '"') {
                    tacc_dynstring_push(ret_tok_str, '\\');
                    tacc_dynstring_push(ret_tok_str, '"');
                } else if (ch == '\\') {
                    tacc_dynstring_push(ret_tok_str, '\\');
                    tacc_dynstring_push(ret_tok_str, '\\');
                } else {
                    tacc_dynstring_push(ret_tok_str, ch);
                }
            }
        } else {
            this_tok_str = tok->str;
            tacc_dynstring_join(ret_tok_str, this_tok_str);
        }
        tok_entry = tok_entry + tacc_sizeadj(1, sizeof(struct tacc_token_p));
    }
    tacc_dynstring_push(ret_tok_str, '"');

    return ret;
}

static void tacc_tok_iter_push_all_expanding(tacc_tok_iter_p iter,
                                             tacc_token_pp tokens,
                                             size_t num_tokens,
                                             tacc_bool first_preceded_by_ws,
                                             tacc_macro_def_p macro_def) {
    tacc_tok_iter_p helper_iter;
    pp_tok_p helper_eof;
    pp_tok_p new_tok;
    pp_tok_p tok;
    tacc_token_pp token_cur;
    tacc_token_pp reverse_toks;
    size_t i;
    size_t tok_count;

    helper_iter = tacc_tok_iter_new();
    helper_iter->file_iter = iter->file_iter;
    helper_iter->inc_level = 0;
    helper_iter->skip_level = 0;
    helper_iter->override = NULL;
    helper_iter->state = iter->state;
    helper_iter->in_macro_args = 0;

    helper_iter->pending =
        tacc_malloc(512 * sizeof(struct tacc_token_p));
    helper_iter->pending_len = 0;

    helper_eof = tacc_pp_tok_new();
    tacc_pp_tok_init(helper_eof);
    helper_eof->kind = TOK_EOF;
    helper_eof->is_final = 1;
    helper_eof->preceded_by_ws = 1;
    tacc_tok_iter_push_pending(helper_iter, helper_eof);

    /* Soak the sponge */
    token_cur = tokens + tacc_sizeadj(num_tokens, sizeof(struct tacc_token_p));
    for (i = 0; i < num_tokens; i = i + 1) {
        token_cur = token_cur - tacc_sizeadj(1, sizeof(struct tacc_token_p));
        tacc_tok_iter_push_pending(helper_iter,
                                   token_cur->content);
    }

    /* Squeeze the sponge */
    macro_def->is_replacing = 0;
    reverse_toks = tacc_malloc(512 * sizeof(struct tacc_token_p));
    tok_count = 0;
    while (1) {
        tok = tacc_tok_iter_next(helper_iter);
        if (tok->kind == TOK_EOF) {
            break;
        }
        if ((tok_count == 0) &&
            (tok->preceded_by_ws != first_preceded_by_ws)) {
            new_tok = tacc_pp_tok_clone(tok);
            new_tok->preceded_by_ws = first_preceded_by_ws;
            tok = new_tok;
        }
        reverse_toks->content = tok;
        reverse_toks =
            reverse_toks + tacc_sizeadj(1, sizeof(struct tacc_token_p));
        tok_count = tok_count + 1;
    }
    /* Reverse order, for buffer */
    for (i = 0; i < tok_count; i = i + 1) {
        reverse_toks =
            reverse_toks - tacc_sizeadj(1, sizeof(struct tacc_token_p));
        tacc_tok_iter_push_pending(iter, reverse_toks->content);
    }
    macro_def->is_replacing = 1;
}

static void tacc_pp_macro_def_func_expand(tacc_tok_iter_p iter_within,
                                          tacc_macro_def_p macro_def,
                                          tacc_token_pp arg_list,
                                          size_t arg_list_size) {
    /*
     * for replacing_tok in replacement list, last to first:
     *   assert replacing_tok != TOK_SHARP (constraint 6.10.3.2p1)
     *   if replacing_tok = TOK_SHARP_2 then
     *     consume(TOK_SHARP_2)
     *     assert more left in replacement list
     *     tok := next token in replacement list
     *     if tok is not ident
     *       push tok, joining
     *     par := macro_def.params[tok.ident]
     *     if par not present
     *       push tok, joining
     *       continue
     *     arg := arg_list[position of par]
     *     if arg is empty
     *       continue
     *
     *     --[ form: param ## stack ]--
     *     push arg[len-1], joining
     *     push arg[len-2 to 0], without joining, EXPANDING
     *     continue
     *   if replacing_tok != TOK_IDENT then
     *     push replacing_tok
     *     continue
     *   par := macro_def.params[replacing_tok.ident]
     *   if par not present
     *     push replacing_tok
     *     continue
     *   arg := arg_list[position of par]
     *   if no more left in replacement list
     *     push args, EXPANDING
     *     continue
     *   tok := next token in replacement list
     *   if tok = TOK_SHARP_2
     *     --[ form: ... ## param stack ]--
     *     do not (!) consume TOK_SHARP_2
     *     push arg[len-1 to 1], EXPANDING
     *     push arg[0], NOT EXPANDING
     *     continue
     *   if tok = TOK_SHARP
     *     str := stringify ALL OF args, NOT EXPANDING
     *     push TOK_STRING(str)
     *     continue
     *   push args, EXPANDING
     */

    pp_tok_p replacing_tok;
    tacc_token_pp replacing_tok_entry;
    pp_tok_p next_tok;
    tacc_token_pp next_tok_entry;
    size_t i;
    size_t par_position;
    tacc_token_p_list_p split_args;
    tacc_token_p_list_p arg_entry;
    tacc_token_pp arg;
    tacc_token_pp arg_last_tok;

    split_args = tacc_pp_split_args(macro_def, arg_list, arg_list_size);

    replacing_tok_entry =
        macro_def->replacement_list +
        tacc_sizeadj(macro_def->replacement_list_len,
                     sizeof(struct tacc_token_p));
    for (i = 0; i < macro_def->replacement_list_len; i = i + 1) {
        replacing_tok_entry =
            replacing_tok_entry - tacc_sizeadj(1, sizeof(struct tacc_token_p));
        replacing_tok = replacing_tok_entry->content;
        tacc_assert(replacing_tok->kind != TOK_SHARP,
                    "stray # in function-like macro expansion list");
        if (replacing_tok->kind == TOK_SHARP_2) {
            i = i + 1;
            replacing_tok_entry = replacing_tok_entry -
                                  tacc_sizeadj(1, sizeof(struct tacc_token_p));
            tacc_assert(i < macro_def->replacement_list_len,
                        "## on edge of function-like macro expansion list");
            next_tok = replacing_tok_entry->content;
            if (next_tok->kind != TOK_IDENT) {
                tacc_tok_iter_join_pending(iter_within, next_tok);
                continue;
            }
            par_position = tacc_macro_def_index_of_par(
                macro_def, tacc_dynstring_as_str(next_tok->str));
            if (par_position == TACC_PARAM_NOT_FOUND) {
                tacc_tok_iter_join_pending(iter_within, next_tok);
                continue;
            }
            arg_entry =
                split_args +
                tacc_sizeadj(par_position, sizeof(struct tacc_token_p_list));
            arg = arg_entry->list;
            if (arg_entry->list_len == 0) {
                /*
                 * There must be a previous token (which is possibly a
                 * placemarker, or a real token). Do not push a placemarker.
                 */
                continue;
            }
            arg_last_tok =
                arg + tacc_sizeadj(arg_entry->list_len - 1,
                                   sizeof(struct tacc_token_p));
            tacc_tok_iter_join_pending(iter_within,
                                       arg_last_tok->content);
            if (arg_entry->list_len > 1) {
                tacc_tok_iter_push_all_expanding(
                    iter_within,
                    arg,
                    arg_entry->list_len - 1,
                    next_tok->preceded_by_ws,
                    macro_def);
            }
            continue;
        }
        if (replacing_tok->kind != TOK_IDENT) {
            tacc_tok_iter_push_pending(iter_within, replacing_tok);
            continue;
        }
        par_position = tacc_macro_def_index_of_par(
            macro_def, tacc_dynstring_as_str(replacing_tok->str));
        if (par_position == TACC_PARAM_NOT_FOUND) {
            tacc_tok_iter_push_pending(iter_within, replacing_tok);
            continue;
        }
        arg_entry = split_args + tacc_sizeadj(par_position,
                                              sizeof(struct tacc_token_p_list));
        arg = arg_entry->list;
        if (i == macro_def->replacement_list_len - 1) {
            if (arg_entry->list_len == 0) {
                /* nothing to do */
                continue;
            }
            tacc_tok_iter_push_all_expanding(iter_within,
                                             arg,
                                             arg_entry->list_len,
                                             0,
                                             macro_def);
            continue;
        }
        next_tok_entry =
            replacing_tok_entry - tacc_sizeadj(1, sizeof(struct tacc_token_p));
        next_tok = next_tok_entry->content;
        if (next_tok->kind == TOK_SHARP_2) {
            if (arg_entry->list_len == 0) {
                tacc_tok_iter_push_placemarker(iter_within);
                continue;
            }
            tacc_tok_iter_push_all_expanding(
                iter_within,
                arg + tacc_sizeadj(1, sizeof(struct tacc_token_p)),
                arg_entry->list_len - 1,
                replacing_tok->preceded_by_ws,
                macro_def);
            tacc_tok_iter_push_pending(iter_within, arg->content);
            continue;
        }
        if (next_tok->kind == TOK_SHARP) {
            next_tok = tacc_pp_stringify(arg, arg_entry->list_len);
            if (iter_within->in_include_directive) {
                next_tok->kind = TOK_INCDIR_STRING;
            }
            tacc_tok_iter_push_pending(iter_within, next_tok);

            /* consume */
            i = i + 1;
            replacing_tok_entry = replacing_tok_entry -
                                  tacc_sizeadj(1, sizeof(struct tacc_token_p));

            continue;
        }
        if (arg_entry->list_len == 0) {
            /* nothing to do */
            continue;
        }
        tacc_tok_iter_push_all_expanding(iter_within,
                                         arg,
                                         arg_entry->list_len,
                                         replacing_tok->preceded_by_ws,
                                         macro_def);
    }
}

static pp_tok_p tacc_tok_iter_peek_handle_macros(tacc_tok_iter_p iter) {
    /* clang-format off */
    /*
     * peek with macros - needs a double peek buffer - never goes across files:
     * redo:
     *  p := peek without macros()
     *  if p != ident then
     *    return p
     *  if p is not defined as macro then
     *    return p
     *  if p defined as object-like then
     *    consume()
     *    add tokens of expand(p) to token queue. they can be expanded again
     *    goto redo
     *  if p = ident and p is not defined as function-like then
     *    return p
     *  if p = ident and p defined as function-like then
     *    consume()
     *    p' := peek, without macros (!!!), deeper()
     *    if p' != lparen
     *      unconsume(p)
     *      return p
     *    consume()
     *    nest_level := 1
     *    tokens := []
     *    while true:
     *      p' := consume()
     *      if p' = lparen:
     *        nest_level += 1
     *      if p' = rparen:
     *        nest_level -= 1
     *      if nest_level = 0:
     *        break
     *      tokens += p'
     *    add tokens of expand(p, token) to token queue. they can be expanded again.
     *    goto redo
     * return p
     */
    /* clang-format off */
    pp_tok_p tok;
    pp_tok_p new_tok;
    tacc_token_pp src_tok_entry;
    tacc_token_pp arg_list;
    tacc_token_pp arg_list_cur;
    tacc_macro_def_entry_p macro_entry;
    tacc_macro_def_p macro_def;
    size_t i;
    size_t nest_level;

    while (1) {
        tok = tacc_tok_iter_peek_nomacro(iter);
        if (tok->kind != TOK_IDENT) {
            return tok;
        }
        if (tok->is_final) {
            return tok;
        }

        macro_entry = tacc_pp_find_macro_or_first_empty(
            iter->state, tacc_dynstring_as_str(tok->str));
        macro_def = macro_entry->content;
        if (!macro_def) {
            return tok;
        }
        if (macro_def->is_tombstone) {
            return tok;
        }

        iter->pending_len = iter->pending_len - 1;

        if (!macro_def->is_function_like) {
            macro_def->is_replacing = 1;

            tacc_tok_iter_insert_macro_replacing_stop(
                iter, tacc_dynstring_as_str(macro_def->name));
            src_tok_entry =
                macro_def->replacement_list +
                tacc_sizeadj(macro_def->replacement_list_len - 1,
                             sizeof(struct tacc_token_p));
            for (i = 0; i < macro_def->replacement_list_len;
                 i = i + 1) {
                if (src_tok_entry->content->kind ==
                    TOK_SHARP_2) {
                    tacc_assert(i != 0,
                                "unexpected ## at beginning of replacement "
                                "list of object-like macro");
                    tacc_assert(
                        i != macro_def->replacement_list_len - 1,
                        "unexpected ## at end of replacement list of "
                        "object-like macro");

                    i = i + 1;
                    src_tok_entry =
                        src_tok_entry -
                        tacc_sizeadj(1, sizeof(struct tacc_token_p));
                    tacc_tok_iter_join_pending(
                        iter, src_tok_entry->content);
                } else {
                    tacc_tok_iter_push_pending(
                        iter, src_tok_entry->content);
                }

                src_tok_entry = src_tok_entry -
                                tacc_sizeadj(1, sizeof(struct tacc_token_p));
            }
            continue;
        }
        new_tok = tacc_tok_iter_peek_nomacro(iter);
        if (new_tok->kind != TOK_LPAREN) {
            /*
             * Not matched as a function-like macro; push `tok` back into queue.
             * To prevent an infinite loop, immediately return `tok`.
             */
            tacc_tok_iter_push_pending(iter, tok);
            return tok;
        }

        /* consume new_tok (lparen) */
        iter->pending_len = iter->pending_len - 1;

        arg_list = tacc_malloc(512 * sizeof(struct tacc_token_p));
        nest_level = 1;
        i = 0;
        arg_list_cur = arg_list;
        while (nest_level > 0) {
            /*
             * Directives inside macro argument list are UB, so do not search
             * for them. This also permits TOK_SHARP to be scanned, for
             * stringification.
             */
            iter->in_macro_args = 1;
            tok = tacc_tok_iter_consume_nomacro(iter);
            iter->in_macro_args = 0;
            tacc_assert(tok->kind != TOK_EOF,
                        "unmatched paren while invoking function-like macro");
            if (tok->kind == TOK_LPAREN) {
                nest_level = nest_level + 1;
            }
            if (tok->kind == TOK_RPAREN) {
                nest_level = nest_level - 1;
            }
            if (nest_level != 0) {
                arg_list_cur->content = tok;
                i = i + 1;
            }
            arg_list_cur =
                arg_list_cur + tacc_sizeadj(1, sizeof(struct tacc_token_p));
        }

        tacc_tok_iter_insert_macro_replacing_stop(
            iter, tacc_dynstring_as_str(macro_def->name));
        macro_def->is_replacing = 1;

        tacc_pp_macro_def_func_expand(iter, macro_def, arg_list, i);
    }
}

static pp_tok_p tacc_tok_iter_peek_handle_directives(tacc_tok_iter_p first) {
    tacc_tok_iter_p last_iter;
    tacc_tok_iter_p prev_iter;
    pp_tok_p peek_tok;

    while (1) {
        /* find deepest level of expansion */
        last_iter = first;
        prev_iter = NULL;
        while (last_iter->override) {
            prev_iter = last_iter;
            last_iter = last_iter->override;
        }

        /* ensure there is a token saved in peek buffer */
        peek_tok = tacc_tok_iter_peek_handle_macros(last_iter);
        if (peek_tok->kind == TOK_EOF) {
            tacc_assert(last_iter->inc_level == 0,
                        "missing #endif, including at level %d",
                        last_iter->inc_level);
            tacc_assert(last_iter->skip_level == 0,
                        "missing #endif, skipping at level %d",
                        last_iter->skip_level);
            /* EOF of current file; consume */
            if (!prev_iter) {
                /* base case: completely EOF, return EOF */
                return peek_tok;
            }

            /* go up a level */
            last_iter = prev_iter;
            last_iter->override = NULL;

            /* return to scanning from first iterator */
            continue;
        }
        if (peek_tok->kind != TOK_DIRECTIVE) {
            /* not a directive, pass through */
            return peek_tok;
        }

        /* going to handle a directive, so it's no longer pending */
        tacc_tok_iter_consume_nomacro(last_iter);

        /* Handle directive. This will never skip over any tokens. */
        tacc_tok_iter_handle_directive(
            first, peek_tok->str);
    }
}

pp_tok_p tacc_tok_iter_peek(tacc_tok_iter_p iter) {
    return tacc_tok_iter_peek_handle_directives(iter);
}

pp_tok_p tacc_tok_iter_next(tacc_tok_iter_p iter) {
    tacc_tok_iter_p level;
    pp_tok_p ret;

    ret = tacc_tok_iter_peek(iter);

    level = tacc_tok_iter_cur_iter(iter);
    tacc_tok_iter_consume_nomacro(level);

    return ret;
}

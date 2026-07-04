#include "tasku_pp.h"
#include "dynarray.h"
#include "dynstring.h"
#include "expr.h"
#include "machine.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if __M2__
#define TACC_PARAM_NOT_FOUND 0xFFFFFFFF
#else
#define TACC_PARAM_NOT_FOUND SIZE_MAX
#endif

void tacc_file_iter_free(struct tacc_file_iter *iter) {
    if (iter->filename) {
        tacc_free(iter->filename);
    }
    tacc_free(iter->orig);
    tacc_free(iter);
}

/* iter: borrow, start: owning, end: owning */
static void tacc_file_iter_init_str(struct tacc_file_iter *iter,
                                    char *start,
                                    char *end) {
    iter->is_bol = 1;
    iter->is_ws = 1;
    iter->orig = start;
    iter->src = start;
    iter->end = end;
    iter->filename = NULL;
}

/* iter: borrow, file: owning */
void tacc_file_iter_init(struct tacc_file_iter *iter, struct tacc_file *file) {
    tacc_file_iter_init_str(iter, file->src, file->src + file->len);
    iter->filename = file->name;
    tacc_free(file);
}

/* return: owning */
struct tacc_file_iter *tacc_file_iter_new_file(struct tacc_file *file) {
    struct tacc_file_iter *iter;

    iter = tacc_malloc(sizeof(struct tacc_file_iter));
    tacc_file_iter_init(iter, file);

    return iter;
}

/* return: owning */
struct tacc_file_iter *tacc_file_iter_new_str(char *start, char *end) {
    struct tacc_file_iter *iter;

    iter = tacc_malloc(sizeof(struct tacc_file_iter));
    tacc_file_iter_init_str(iter, start, end);

    return iter;
}

/* tok: borrow */
void tacc_pp_tok_init(struct pp_tok *tok) {
    tok->kind = TOK_UNRECOGNIZED;
    tok->ident_kind = ID_OTHER;
    tok->is_final = 0;
    tok->preceded_by_ws = 0;
    tok->str = tacc_dynstring_new();
}

/* return: owning */
struct pp_tok *tacc_pp_tok_new(void) {
    struct pp_tok *tok;

    tok = tacc_malloc(sizeof(struct pp_tok));
    tacc_pp_tok_init(tok);

    return tok;
}

/* return: owning, tok: borrow */
struct pp_tok *tacc_pp_tok_clone(struct pp_tok *tok) {
    struct pp_tok *new_tok;

    new_tok = tacc_malloc(sizeof(struct pp_tok));

    new_tok->kind = tok->kind;
    new_tok->ident_kind = tok->ident_kind;
    new_tok->is_final = tok->is_final;
    new_tok->preceded_by_ws = tok->preceded_by_ws;
    new_tok->str = tacc_dynstring_clone(tok->str);

    return new_tok;
}

void tacc_pp_tok_free(struct pp_tok *tok) {
    tacc_dynstring_free(tok->str);
    tacc_free(tok);
}

/* return: static, tok: borrow */
char *tacc_pp_to_string(struct pp_tok *tok) {
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

/* iter: borrow */
static void tacc_file_iter_eat_splices(struct tacc_file_iter *iter) {
    char ch;
    char *src;

    src = iter->src;
    ch = *src;

    if (ch == '\\') {
        /* c99 5.1.1.2p2: a physical source file cannot end with a backslash */
        ch = iter->src[1];
        if (ch == '\n') {
            iter->src = iter->src + 1;
            iter->src = iter->src + 1;
        }
    }
}

/* return: borrow, iter: borrow */
static char *tacc_file_iter_cur(struct tacc_file_iter *iter) {
    tacc_file_iter_eat_splices(iter);
    return iter->src;
}

/* iter: borrow */
tacc_bool tacc_file_is_eof(struct tacc_file_iter *iter) {
    if (tacc_file_iter_cur(iter) == iter->end) {
        return 1;
    }
    return 0;
}

/* iter: borrow */
char tacc_file_iter_peek_ch(struct tacc_file_iter *iter) {
    char *cur;

    cur = tacc_file_iter_cur(iter);
    tacc_assert(!tacc_file_is_eof(iter), "eof while peeking");
    return *cur;
}

/* iter: borrow */
char tacc_file_iter_consume_ch(struct tacc_file_iter *iter) {
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

/* iter: borrow */
tacc_bool tacc_file_iter_accept_ch(struct tacc_file_iter *iter, char accept) {
    if (tacc_file_is_eof(iter)) {
        return 0;
    }
    if (tacc_file_iter_peek_ch(iter) == accept) {
        tacc_file_iter_consume_ch(iter);
        return 1;
    }
    return 0;
}

/* iter: borrow */
static void tacc_file_iter_eat_comment(struct tacc_file_iter *iter) {
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

/* iter: borrow */
static void tacc_file_iter_eat_new_comment(struct tacc_file_iter *iter) {
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

/* iter: borrow */
static void tacc_file_iter_eat_ws_no_newlines(struct tacc_file_iter *iter) {
    char ch;
    char *src;
    while (1) {
        if (tacc_file_iter_accept_ch(iter, ' ')) {
            continue;
        }
        if (tacc_file_iter_accept_ch(iter, 9)) {
            continue;
        }
        if ((iter->end - iter->src) >= 2) {
            src = iter->src;
            ch = *src;
            if (ch == '/') {
                ch = src[1];
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

/* iter: borrow */
static void tacc_file_iter_eat_all_ws(struct tacc_file_iter *iter) {
    char ch;
    char *src;
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
            src = iter->src;
            ch = *src;
            if (ch == '/') {
                ch = src[1];
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

/* iter: borrow, str: borrow */
static void tacc_file_iter_lex_escape(struct tacc_file_iter *iter,
                                      struct tacc_string *str) {
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

/* tok: borrow, str: owning */
static void tacc_pp_tok_assign_tstr(struct pp_tok *tok,
                                    struct tacc_string *str) {
    if (tok->str != NULL) {
        tacc_dynstring_free(tok->str);
    }
    tok->str = str;
}

/* tok: borrow, str: borrow */
static void tacc_pp_tok_assign_str(struct pp_tok *tok, char *str) {
    if (tok->str == NULL) {
        tok->str = tacc_dynstring_new();
    } else {
        tacc_dynstring_reset(tok->str);
    }
    tacc_dynstring_concat(tok->str, str);
}

/* iter: borrow, tok_out: borrow */
static tacc_bool tacc_file_iter_lex_directive(struct tacc_file_iter *iter,
                                              struct pp_tok *tok_out) {
    struct tacc_string *directive_str;
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

    tacc_pp_tok_assign_tstr(tok_out, directive_str);
    tok_out->kind = TOK_DIRECTIVE;

    return 1;
}

/* return: owning, iter: borrow, tok_out: owning */
static struct pp_tok *tacc_file_iter_lex_char(struct tacc_file_iter *iter,
                                              struct pp_tok *tok_out) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    struct pp_tok *ret = tok_out;
    char contained;
    struct tacc_string *out_str;

    out_str = tacc_dynstring_new();
    ret->kind = TOK_CHAR;
    tacc_dynstring_push(out_str, '\'');

    tacc_assert(!tacc_file_iter_accept_ch(iter, '\''),
                "empty character literal");
    if (!tacc_file_iter_accept_ch(iter, '\\')) {
        contained = tacc_file_iter_consume_ch(iter);
        tacc_assert(tacc_file_iter_accept_ch(iter, '\''),
                    "overlong character literal");

        tacc_dynstring_push(out_str, contained);
        tacc_dynstring_push(out_str, '\'');
        tacc_pp_tok_assign_tstr(ret, out_str);
        return ret;
    }
    tacc_file_iter_lex_escape(iter, out_str);
    tacc_assert(tacc_file_iter_accept_ch(iter, '\''),
                "overlong character literal");
    tacc_dynstring_push(out_str, '\'');
    tacc_pp_tok_assign_tstr(ret, out_str);

    return ret;
}

/* return: owning, iter: borrow, tok_out: owning */
static struct pp_tok *tacc_file_iter_lex_string(struct tacc_file_iter *iter,
                                                struct pp_tok *tok_out) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    struct pp_tok *ret = tok_out;
    struct tacc_string *out_str;

    ret->kind = TOK_STRING;

    out_str = tacc_dynstring_new();
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
    tacc_pp_tok_assign_tstr(ret, out_str);

    return ret;
}

static void tacc_file_iter_eat_literal(struct tacc_file_iter *iter) {
    struct pp_tok to_forget;

    if (tacc_file_iter_accept_ch(iter, '\'')) {
        tacc_file_iter_lex_char(iter, &to_forget);
        return;
    }
    if (tacc_file_iter_accept_ch(iter, '\"')) {
        tacc_file_iter_lex_string(iter, &to_forget);
        return;
    }
}

/* return: owning, iter: borrow, tok_out: owning */
static struct pp_tok *tacc_file_iter_lex_incfile(struct tacc_file_iter *iter,
                                                 struct pp_tok *tok_out,
                                                 char first) {
    struct pp_tok *ret = tok_out;
    struct tacc_string *out_str;
    char last;

    if (first == '"') {
        last = '"';
    } else {
        last = '>';
    }

    out_str = tacc_dynstring_new();
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
    tacc_pp_tok_assign_tstr(ret, out_str);

    return ret;
}

/* return: owning, iter: borrow, tok_out: owning */
static struct pp_tok *tacc_file_iter_lex_ppnum(struct tacc_file_iter *iter,
                                               struct pp_tok *tok_out,
                                               char first) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    struct pp_tok *ret = tok_out;
    struct tacc_string *out_str;
    char ch;

    ret->kind = TOK_PPNUM;

    out_str = tacc_dynstring_new();
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
    tacc_pp_tok_assign_tstr(ret, out_str);

    return ret;
}

/* ident: borrow */
enum pp_ident_kind tacc_recognize_ident_kind(char *ident) {
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

/* return: owning, iter: borrow, tok_out: owning */
static struct pp_tok *tacc_file_iter_lex_ident(struct tacc_file_iter *iter,
                                               struct pp_tok *tok_out,
                                               char first) {
    struct pp_tok *ret = tok_out;
    struct tacc_string *out_str;
    char ch;

    ret->kind = TOK_IDENT;

    out_str = tacc_dynstring_new();
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
    ret->ident_kind = tacc_recognize_ident_kind(tacc_dynstring_as_str(out_str));
    tacc_pp_tok_assign_tstr(ret, out_str);

    return ret;
}

/* iter: borrow, tok_out: borrow, special_match: borrow */
static tacc_bool tacc_file_iter_maybe_special(struct tacc_file_iter *iter,
                                              struct pp_tok *tok_out,
                                              enum pp_tok_kind special_tok,
                                              char *special_match) {
    struct tacc_string *out_str;

    if (!tacc_file_iter_accept_ch(iter, special_match[1])) {
        out_str = tacc_dynstring_new();
        tacc_dynstring_push(out_str, special_match[0]);
        tacc_pp_tok_assign_tstr(tok_out, out_str);
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

/* return: owning, iter: borrow */
static struct pp_tok *tacc_file_iter_lex(struct tacc_file_iter *iter,
                                         tacc_lex_context_e ctx) {
    char first;
    char ch;
    struct pp_tok *ret;
    enum pp_tok_kind kind;

    ret = tacc_pp_tok_new();
    if (ctx == LEX_SKIPPING) {
        while (1) {
            if (tacc_file_is_eof(iter)) {
                ret->kind = TOK_EOF;
                tacc_pp_tok_assign_str(ret, "");
                return ret;
            }

            tacc_file_iter_eat_ws_no_newlines(iter);

            ch = tacc_file_iter_peek_ch(iter);
            if (ch == '"' || ch == '\'') {
                tacc_file_iter_eat_literal(iter);
                continue;
            }
            if (ch != '\n' && ch != '#') {
                tacc_file_iter_consume_ch(iter);
                continue;
            }

            if (tacc_file_iter_lex_directive(iter, ret)) {
                return ret;
            }
            /* encountered a # that was not in a valid directive? */
            tacc_file_iter_accept_ch(iter, '#');
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
        ret->kind = TOK_SHARP;
        tacc_file_iter_maybe_special(iter, ret, TOK_SHARP_2, "##");
        return ret;
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
        if (first == '.' && ((iter->end - iter->src) >= 2) &&
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

/* return: owning, iter: borrow */
static struct pp_tok *tacc_file_iter_expect_ident(struct tacc_file_iter *iter) {
    struct pp_tok *tok;
    char ch;

    tok = tacc_pp_tok_new();
    ch = tacc_file_iter_consume_ch(iter);
    tacc_assert((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch == '_'),
                "expected identifier, got %x",
                ch);
    tacc_file_iter_lex_ident(iter, tok, ch);

    return tok;
}

/* state: borrow */
void tacc_pp_state_init(struct tacc_pp_state *state,
                        struct tacc_target *target) {
    struct tacc_string *thisdir_incpath;

    state->target = target;

    state->include_path = tacc_string_list_new();
    thisdir_incpath = tacc_dynstring_new();
    tacc_dynstring_concat(thisdir_incpath, ".");
    tacc_string_list_push(state->include_path, thisdir_incpath);
    state->macros = tacc_macro_def_list_new();

    tacc_pp_define(state, "__STDC__", "1");
}

/* return: owning */
struct tacc_pp_state *tacc_pp_state_new(struct tacc_target *target) {
    struct tacc_pp_state *state;

    state = tacc_malloc(sizeof(struct tacc_pp_state));
    tacc_pp_state_init(state, target);

    return state;
}

/* return: borrow, state: borrow, name: borrow */
struct tacc_macro_def_list_entry *
tacc_pp_find_macro_or_first_empty(struct tacc_pp_state *state, char *name) {
    size_t i;
    struct tacc_macro_def_list_entry *entry;

    for (i = 0; i < tacc_macro_def_list_len(state->macros); i = i + 1) {
        entry = tacc_macro_def_list_get(state->macros, i);
        if (!entry->content) {
            /* first empty */
            return entry;
        }
        if (!strcmp(tacc_dynstring_as_str(entry->content->name), name)) {
            /* match, possibly tombstone */
            return entry;
        }
    }
    tacc_macro_def_list_push(state->macros, NULL);
    return tacc_macro_def_list_get(state->macros,
                                   tacc_macro_def_list_len(state->macros) - 1);
}

static tacc_bool tacc_pp_macro_is_defined(struct tacc_pp_state *state,
                                          char *name) {
    struct tacc_macro_def_list_entry *entry;
    entry = tacc_pp_find_macro_or_first_empty(state, name);
    if (!entry->content) {
        return 0;
    }
    if (entry->content->is_tombstone) {
        return 0;
    }
    return 1;
}

static void tacc_macro_def_free(struct tacc_macro_def *macro_def) {
    if (macro_def == NULL) {
        return;
    }
    tacc_dynstring_free(macro_def->name);
    if (macro_def->params != NULL) {
        tacc_string_list_free(macro_def->params);
        tacc_free(macro_def->params);
    }
    tacc_token_list_free(macro_def->replacement_list);
    tacc_free(macro_def->replacement_list);
    tacc_free(macro_def);
}

MK_DYNARRAY_OVER(tacc_string_list,
                 tacc_string_list_entry,
                 struct tacc_string *,
                 tacc_string_list_new,
                 tacc_string_list_init,
                 tacc_string_list_get,
                 tacc_string_list_push,
                 tacc_string_list_pop,
                 tacc_string_list_len,
                 tacc_dynstring_free,
                 tacc_string_list_free)

MK_DYNARRAY_OVER(tacc_token_list,
                 tacc_token_list_entry,
                 struct pp_tok *,
                 tacc_token_list_new,
                 tacc_token_list_init,
                 tacc_token_list_get,
                 tacc_token_list_push,
                 tacc_token_list_pop,
                 tacc_token_list_len,
                 tacc_pp_tok_free,
                 tacc_token_list_free)

MK_DYNARRAY_OVER(tacc_macro_def_list,
                 tacc_macro_def_list_entry,
                 struct tacc_macro_def *,
                 tacc_macro_def_list_new,
                 tacc_macro_def_list_init,
                 tacc_macro_def_list_get,
                 tacc_macro_def_list_push,
                 tacc_macro_def_list_pop,
                 tacc_macro_def_list_len,
                 tacc_macro_def_free,
                 tacc_macro_def_list_free)

/* state: borrow, macro: owning */
void tacc_pp_insert_macro(struct tacc_pp_state *state,
                          struct tacc_macro_def *macro) {
    struct tacc_macro_def_list_entry *place;

    place = tacc_pp_find_macro_or_first_empty(
        state, tacc_dynstring_as_str(macro->name));
    if (place->content) {
        if (!place->content->is_tombstone) {
            printf("warning: macro defined twice: %s", macro->name->string);
        }
        tacc_macro_def_free(place->content);
    }
    place->content = macro;
}

/* state: borrow, name: borrow, expansion: borrow */
void tacc_pp_define(struct tacc_pp_state *state, char *name, char *expansion) {
    struct tacc_token_list *replacement_list;
    struct tacc_macro_def *macro;
    struct tacc_file_iter *iter;
    struct pp_tok *tok;
    char *expansion_owned;

    macro = tacc_malloc(sizeof(struct tacc_macro_def));
    macro->name = tacc_dynstring_new();
    tacc_dynstring_concat(macro->name, name);
    macro->is_va = 0;
    macro->is_tombstone = 0;
    macro->is_function_like = 0;
    macro->params = NULL;
    macro->is_replacing = 0;

    replacement_list = tacc_token_list_new();
    macro->replacement_list = replacement_list;

    expansion_owned = tacc_malloc(strlen(expansion) + 1);
    strcpy(expansion_owned, expansion);
    iter = tacc_file_iter_new_str(expansion_owned,
                                  expansion_owned + strlen(expansion));

    while (1) {
        tok = tacc_file_iter_lex(iter, LEX_IN_REPLACEMENT_LIST);
        if (tok->kind == TOK_EOF) {
            tacc_pp_tok_free(tok);
            break;
        }
        tacc_token_list_push(replacement_list, tok);
    }

    tacc_pp_insert_macro(state, macro);

    tacc_file_iter_free(iter);
}

/* state: borrow, name: borrow */
void tacc_pp_undef(struct tacc_pp_state *state, char *name) {
    struct tacc_macro_def_list_entry *place;

    place = tacc_pp_find_macro_or_first_empty(state, name);
    if (place->content == NULL) {
        return;
    }
    place->content->is_tombstone = 1;
}

/* iter: borrow, file: owning, possibly null, state: borrow */
void tacc_tok_iter_init(struct tacc_tok_iter *iter,
                        struct tacc_file_iter *file,
                        struct tacc_pp_state *state) {
    iter->file_iter = file;
    iter->pending = tacc_token_list_new();
    iter->override = NULL;
    iter->state = state;
    iter->inc_level = 0;
    iter->skip_level = 0;
    iter->in_macro_args = 0;
    iter->in_include_directive = 0;
    iter->in_if = 0;
    iter->skip_till_endif = 0;
}

/* return: owning */
struct tacc_tok_iter *tacc_tok_iter_new(struct tacc_file_iter *file,
                                        struct tacc_pp_state *state) {
    struct tacc_tok_iter *iter;

    iter = tacc_malloc(sizeof(struct tacc_tok_iter));
    tacc_tok_iter_init(iter, file, state);

    return iter;
}

/* return: owning, base: borrow, subpath: borrow */
static struct tacc_file *tacc_pp_try_file(char *base, char *subpath) {
    char *path;
    FILE *file;
    struct tacc_file *opened_file;

    if (strlen(base) + strlen(subpath) >= 254) {
        return NULL;
    }
    path = tacc_malloc(256);
    strcpy(path, base);
    strcat(path, "/");
    strcat(path, subpath);

    file = fopen(path, "r");
    if (!file) {
        tacc_free(path);
        return NULL;
    }
    fclose(file);
    opened_file = tacc_open(path);
    tacc_free(path);
    return opened_file;
}

/* return: owning, state: borrow, cur_file_path: borrow, subpath: borrow */
static struct tacc_file *tacc_pp_search_include_path(
    struct tacc_pp_state *state, char *cur_file_path, char *subpath) {
    /* owning */
    char *cur_file_dirname;
    char *cur_file_path_cur;
    char *cur_file_path_end;
    struct tacc_file *try_file;
    struct tacc_string_list_entry *entry;
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
        tacc_free(cur_file_dirname);
        if (try_file) {
            return try_file;
        }
    }
    for (i = 0; i < tacc_string_list_len(state->include_path); ++i) {
        entry = tacc_string_list_get(state->include_path, i);
        try_file =
            tacc_pp_try_file(tacc_dynstring_as_str(entry->content), subpath);
        if (try_file) {
            return try_file;
        }
    }
    tacc_assert(0, "unable to find suitable file for include: %s", subpath);
    return NULL;
}

/* return: borrow, first: borrow */
static struct tacc_tok_iter *
tacc_tok_iter_cur_iter(struct tacc_tok_iter *first) {
    struct tacc_tok_iter *last_iter;

    last_iter = first;

    while (last_iter->override) {
        last_iter = last_iter->override;
    }
    return last_iter;
}

/* return: borrow, iter: borrow */
static struct pp_tok *
tacc_tok_iter_peek_handle_macros(struct tacc_tok_iter *iter);

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_include(struct tacc_tok_iter *first,
                                         struct tacc_file_iter *iter) {
    char *hdr_name;
    char *hdr_name_start;
    struct pp_tok *tok;
    struct tacc_tok_iter *tok_iter;
    struct tacc_tok_iter *last_iter;
    struct tacc_file *included_file;
    struct tacc_file_iter *included_file_iter;
    struct tacc_tok_iter *included_file_tok_iter;

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_level > 0) {
        tacc_file_iter_free(iter);
        return;
    }

    tok_iter = tacc_tok_iter_new(iter, first->state);
    tok_iter->in_include_directive = 1;

    tok = tacc_tok_iter_next(tok_iter);
    if (tok->kind != TOK_INCDIR_ANGLE) {
        tacc_assert(tok->kind == TOK_INCDIR_STRING,
                    "expected include file string, got %s",
                    tacc_pp_to_string(tok));
    }

    hdr_name = tacc_malloc(1024);
    hdr_name_start = hdr_name;

    tacc_assert(tacc_dynstring_len(tok->str) > 2, "empty include string");
    tacc_assert(tacc_dynstring_len(tok->str) < 1024, "overlong include string");
    strcpy(hdr_name, tacc_dynstring_as_str(tok->str) + 1);
    hdr_name[tacc_dynstring_len(tok->str) - 2] = 0; /* drop > or " */

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_pp_tok_free(tok);
    tok = tacc_tok_iter_next(tok_iter);
    tacc_assert(tok->kind == TOK_EOF, "junk after #include");
    tacc_pp_tok_free(tok);
    tok = NULL;
    tacc_tok_iter_free(tok_iter);

    tacc_assert(last_iter->file_iter != NULL,
                "ICE: cannot use #include from floating tok_iter");
    included_file = tacc_pp_search_include_path(
        first->state, last_iter->file_iter->filename, hdr_name_start);

    tacc_free(hdr_name);

    included_file_iter = tacc_file_iter_new_file(included_file);
    included_file_tok_iter =
        tacc_tok_iter_new(included_file_iter, first->state);
    last_iter->override = included_file_tok_iter;
}

/* first: borrow, iter: borrow */
static void tacc_tok_iter_handle_define(struct tacc_tok_iter *first,
                                        struct tacc_file_iter *iter) {
    struct pp_tok *tok;
    struct tacc_macro_def *macro;
    struct tacc_string_list *params;
    struct tacc_token_list *replacement_list;
    struct tacc_tok_iter *last_iter;
    tacc_bool terminated;

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_level > 0) {
        tacc_file_iter_free(iter);
        return;
    }

    macro = tacc_malloc(sizeof(struct tacc_macro_def));

    tok = tacc_file_iter_expect_ident(iter);
    macro->name = tacc_dynstring_clone(tok->str);
    tacc_pp_tok_free(tok);
    tok = NULL;

    macro->is_va = 0;
    macro->is_tombstone = 0;
    macro->is_function_like = 0;
    macro->is_replacing = 0;
    macro->params = NULL;

    /* don't eat whitespace before lparen */
    if (tacc_file_iter_accept_ch(iter, '(')) {
        macro->is_function_like = 1;
        tacc_file_iter_eat_ws_no_newlines(iter);
        /* function-like macro */
        params = tacc_string_list_new();
        macro->params = params;
        if (!tacc_file_iter_accept_ch(iter, ')')) {
            terminated = 0;
            while (1) {
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
                    macro->is_va = 1;
                    terminated = 1;
                    break;
                }

                tok = tacc_file_iter_expect_ident(iter);
                tacc_string_list_push(macro->params,
                                      tacc_dynstring_clone(tok->str));
                tacc_pp_tok_free(tok);
                tok = NULL;

                tacc_file_iter_eat_ws_no_newlines(iter);
                if (tacc_file_iter_accept_ch(iter, ',')) {
                    continue;
                }
                tacc_assert(tacc_file_iter_accept_ch(iter, ')'),
                            "expected , or )");
                terminated = 1;
                break;
            }
            tacc_assert(terminated, "overlong macro parameter list");
        }
        tacc_file_iter_eat_ws_no_newlines(iter);
    } else {
        tacc_file_iter_eat_ws_no_newlines(iter);
    }
    /* no ws necessarily required here!!! huh??? */

    replacement_list = tacc_token_list_new();
    macro->replacement_list = replacement_list;
    while (1) {
        tok = tacc_file_iter_lex(iter, LEX_IN_REPLACEMENT_LIST);
        if (tok->kind == TOK_EOF) {
            tacc_pp_tok_free(tok);
            break;
        }
        tacc_token_list_push(replacement_list, tok);
    }
    tacc_pp_insert_macro(first->state, macro);
    tacc_file_iter_free(iter);
}

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_undef(struct tacc_tok_iter *first,
                                       struct tacc_file_iter *iter) {
    struct pp_tok *tok;
    struct tacc_tok_iter *last_iter;

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_level > 0) {
        tacc_file_iter_free(iter);
        return;
    }

    tok = tacc_file_iter_expect_ident(iter);
    tacc_pp_undef(first->state, tacc_dynstring_as_str(tok->str));
    tacc_pp_tok_free(tok);
    tacc_file_iter_free(iter);
}

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_ifndef(struct tacc_tok_iter *first,
                                        struct tacc_file_iter *iter) {
    struct pp_tok *tok;
    struct tacc_tok_iter *last_iter;

    tok = tacc_file_iter_expect_ident(iter);

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #ifndef");

    last_iter = tacc_tok_iter_cur_iter(first);

    if (!tacc_pp_macro_is_defined(first->state,
                                  tacc_dynstring_as_str(tok->str))) {
        last_iter->inc_level = last_iter->inc_level + 1;
        tacc_file_iter_free(iter);
        tacc_pp_tok_free(tok);
        return;
    }
    last_iter->skip_level = last_iter->skip_level + 1;

    tacc_pp_tok_free(tok);
    tacc_file_iter_free(iter);
}

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_ifdef(struct tacc_tok_iter *first,
                                       struct tacc_file_iter *iter) {
    struct pp_tok *tok;
    struct tacc_tok_iter *last_iter;

    tok = tacc_file_iter_expect_ident(iter);

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #ifdef");

    last_iter = tacc_tok_iter_cur_iter(first);

    if (tacc_pp_macro_is_defined(first->state,
                                 tacc_dynstring_as_str(tok->str))) {
        last_iter->inc_level = last_iter->inc_level + 1;
        tacc_file_iter_free(iter);
        tacc_pp_tok_free(tok);
        return;
    }
    last_iter->skip_level = last_iter->skip_level + 1;
    tacc_pp_tok_free(tok);
    tacc_file_iter_free(iter);
}

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_endif(struct tacc_tok_iter *first,
                                       struct tacc_file_iter *iter) {
    struct tacc_tok_iter *last_iter;

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #endif");

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_level > 0) {
        last_iter->skip_level = last_iter->skip_level - 1;
        tacc_file_iter_free(iter);
        return;
    }
    tacc_assert(last_iter->inc_level > 0, "stray #endif");
    last_iter->inc_level = last_iter->inc_level - 1;
    tacc_file_iter_free(iter);

    last_iter->skip_till_endif = 0;
}

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_else(struct tacc_tok_iter *first,
                                      struct tacc_file_iter *iter) {
    struct tacc_tok_iter *last_iter;

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #else");

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_till_endif) {
        /* Skip level should already be set. Don't re-evaluate skipping here, do
         * skip till endif. */
        tacc_file_iter_free(iter);
        return;
    }

    if (last_iter->skip_level == 1) {
        last_iter->skip_level = 0;
        last_iter->inc_level = last_iter->inc_level + 1;
        tacc_file_iter_free(iter);
        return;
    }
    if (last_iter->skip_level > 1) {
        tacc_file_iter_free(iter);
        return;
    }
    tacc_assert(last_iter->inc_level > 0, "stray #else");
    last_iter->inc_level = last_iter->inc_level - 1;
    last_iter->skip_level = last_iter->skip_level + 1;
    tacc_file_iter_free(iter);
}

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_if(struct tacc_tok_iter *first,
                                    struct tacc_file_iter *iter) {
    struct tacc_tok_iter *tok_iter;
    struct tacc_tok_iter *last_iter;
    struct tacc_expr *expr;
    struct tacc_val *val;
    struct pp_tok *tok;

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_level > 0) {
        last_iter->skip_level = last_iter->skip_level + 1;
        tacc_file_iter_free(iter);
        return;
    }

    tacc_file_iter_eat_ws_no_newlines(iter);
    tok_iter = tacc_tok_iter_new(iter, first->state);
    tok_iter->in_if = 1;

    expr = tacc_expr_parse_new(tok_iter);
    val = tacc_expr_const_eval(expr, first->state->target);

    tok = tacc_tok_iter_next(tok_iter);
    tacc_assert(tok->kind == TOK_EOF,
                "junk after #if: %s /*...*/ %s",
                tacc_dynstring_as_str(tok->str),
                tok_iter->file_iter->src);
    tacc_pp_tok_free(tok);
    tacc_assert(tacc_val_is_integral(val),
                "#if argument must evaluate to integer");

    if (tacc_u64_is_zero(val->value.int_value)) {
        last_iter->skip_level = last_iter->skip_level + 1;
    } else {
        last_iter->inc_level = last_iter->inc_level + 1;
    }
    tacc_tok_iter_free(tok_iter);
    tacc_val_free(val);
    tacc_expr_free(expr);
}

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_elif(struct tacc_tok_iter *first,
                                      struct tacc_file_iter *iter) {
    struct tacc_tok_iter *tok_iter;
    struct tacc_tok_iter *last_iter;
    struct tacc_expr *expr;
    struct tacc_val *val;
    struct pp_tok *tok;

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_till_endif) {
        tacc_file_iter_free(iter);
        return;
    }
    if (last_iter->skip_level > 1) {
        tacc_file_iter_free(iter);
        return;
    }
    if (last_iter->skip_level == 0) {
        tacc_assert(last_iter->inc_level > 0,
                    "encountered #elif without corresponding #if/ifndef/ifdef");
        last_iter->skip_level = 1;
        last_iter->inc_level = last_iter->inc_level - 1;
        last_iter->skip_till_endif = 1;
        tacc_file_iter_free(iter);
        return;
    }
    /*
     * we are skipping, and should evaluate whether to stop skipping
     */
    tacc_assert(last_iter->skip_level == 1,
                "encountered #elif without corresponding #if/ifndef/ifdef");

    tacc_file_iter_eat_ws_no_newlines(iter);

    tok_iter = tacc_tok_iter_new(iter, first->state);
    tok_iter->in_if = 1;

    expr = tacc_expr_parse_new(tok_iter);
    val = tacc_expr_const_eval(expr, first->state->target);

    tok = tacc_tok_iter_next(tok_iter);
    tacc_assert(tok->kind == TOK_EOF,
                "junk after #elif: %s /*...*/ %s",
                tacc_dynstring_as_str(tok->str),
                tok_iter->file_iter->src);
    tacc_pp_tok_free(tok);
    tacc_assert(tacc_val_is_integral(val),
                "#elif argument must evaluate to integer");

    if (!tacc_u64_is_zero(val->value.int_value)) {
        last_iter->skip_level = 0;
        last_iter->inc_level = last_iter->inc_level + 1;
    }
    tacc_tok_iter_free(tok_iter);
    tacc_val_free(val);
    tacc_expr_free(expr);
}

/* first: borrow, iter: owning */
static void tacc_tok_iter_handle_error_directive(struct tacc_tok_iter *first,
                                                 struct tacc_file_iter *iter) {
    struct tacc_tok_iter *last_iter;

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->skip_level > 0) {
        tacc_file_iter_free(iter);
        return;
    }

    tacc_file_iter_eat_ws_no_newlines(iter);

    tacc_assert(0, "#error: %s", iter->src);
    tacc_file_iter_free(iter);

#ifdef __STDC__
    (void) first;
#endif
}

/* first: borrow, directive: owning */
static void tacc_tok_iter_handle_directive(struct tacc_tok_iter *first,
                                           struct tacc_string *directive) {
    struct tacc_file_iter *dir_scanner;
    struct tacc_tok_iter *last;
    struct pp_tok *tok;
    char *directive_name;
    char *directive_str;
    size_t directive_len;

    directive_len = tacc_dynstring_len(directive);
    directive_str = tacc_dynstring_take_str(directive);
    tacc_dynstring_free(directive);
    directive = NULL;

    dir_scanner =
        tacc_file_iter_new_str(directive_str, directive_str + directive_len);

    tacc_file_iter_eat_ws_no_newlines(dir_scanner);
    if (tacc_file_is_eof(dir_scanner) ||
        tacc_file_iter_accept_ch(dir_scanner, '\n')) {
        /* empty directive, skip */
        tacc_file_iter_free(dir_scanner);
        return;
    }
    tok = tacc_file_iter_expect_ident(dir_scanner);
    tacc_file_iter_eat_ws_no_newlines(dir_scanner);
    directive_name = tacc_dynstring_take_str(tok->str);
    tacc_pp_tok_free(tok);
    tok = NULL;

    if (!strcmp(directive_name, "include")) {
        tacc_assert(dir_scanner->is_ws, "expected whitespace after #include");
        tacc_tok_iter_handle_include(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "define")) {
        tacc_assert(dir_scanner->is_ws, "expected whitespace after #define");
        tacc_tok_iter_handle_define(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "undef")) {
        tacc_assert(dir_scanner->is_ws, "expected whitespace after #undef");
        tacc_tok_iter_handle_undef(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "if")) {
        tacc_tok_iter_handle_if(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "ifdef")) {
        tacc_assert(dir_scanner->is_ws, "expected whitespace after #ifdef");
        tacc_tok_iter_handle_ifdef(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "ifndef")) {
        tacc_assert(dir_scanner->is_ws, "expected whitespace after #ifndef");
        tacc_tok_iter_handle_ifndef(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "endif")) {
        tacc_tok_iter_handle_endif(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "elif")) {
        tacc_assert(dir_scanner->is_ws, "expected whitespace after #ifndef");
        tacc_tok_iter_handle_elif(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "else")) {
        tacc_tok_iter_handle_else(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }
    if (!strcmp(directive_name, "error")) {
        tacc_tok_iter_handle_error_directive(first, dir_scanner);
        tacc_free(directive_name);
        return;
    }

    last = tacc_tok_iter_cur_iter(first);
    if (last->skip_level > 0) {
        tacc_free(directive_name);
        tacc_file_iter_free(dir_scanner);
        return;
    }

    tacc_assert(0, "unknown directive: %s", directive_name);
}

/* iter: borrow, tok: borrow */
static void tacc_tok_maybe_finalize(struct tacc_tok_iter *iter,
                                    struct pp_tok *tok) {
    struct tacc_macro_def_list_entry *macro_def_list_entry;
    struct tacc_macro_def *macro_def;

    if ((tok->kind != TOK_IDENT) || (tok->is_final)) {
        return;
    }
    macro_def_list_entry = tacc_pp_find_macro_or_first_empty(
        iter->state, tacc_dynstring_as_str(tok->str));
    macro_def = macro_def_list_entry->content;
    if (!macro_def) {
        return;
    }
    if (!macro_def->is_replacing) {
        return;
    }

    tok->is_final = 1;
}

/* return: borrow, iter: borrow */
static struct pp_tok *tacc_tok_iter_peek_nomacro(struct tacc_tok_iter *iter) {
    struct pp_tok *tok;
    struct tacc_macro_def_list_entry *macro_entry;
    struct tacc_macro_def *macro_def;
    struct tacc_token_list_entry *entry;

    if (tacc_token_list_len(iter->pending) == 0) {
        if (iter->file_iter == NULL) {
            tok = tacc_pp_tok_new();
            tok->kind = TOK_EOF;
        } else if (iter->in_macro_args) {
            tok = tacc_file_iter_lex(iter->file_iter, LEX_IN_MACRO_ARGS);
        } else if (iter->skip_level > 0) {
            tok = tacc_file_iter_lex(iter->file_iter, LEX_SKIPPING);
        } else if (iter->in_include_directive) {
            tok = tacc_file_iter_lex(iter->file_iter, LEX_IN_INCLUDE);
        } else {
            tok = tacc_file_iter_lex(iter->file_iter, LEX_TOP_LEVEL);
        }
        tacc_tok_maybe_finalize(iter, tok);
        /* owning store */
        tacc_token_list_push(iter->pending, tok);
        /* borrowing */
        return tok;
    }
    entry = tacc_token_list_get(iter->pending,
                                tacc_token_list_len(iter->pending) - 1);
    tok = entry->content;
    /* ATTN: modifies pending token, even if it is not accepted by caller */
    tacc_tok_maybe_finalize(iter, tok);
    if (tok->kind == TOK_FAKE_END_OF_MACRO) {
        tacc_token_list_pop(iter->pending);

        macro_entry = tacc_pp_find_macro_or_first_empty(
            iter->state, tacc_dynstring_as_str(tok->str));
        macro_def = macro_entry->content;
        tacc_assert(macro_def != NULL,
                    "failed to find macrodef for macro being expanded: %s",
                    tok->str);
        macro_def->is_replacing = 0;

        tacc_pp_tok_free(tok);
        tok = NULL;

        /* simple recursion, this shouldn't go too deep */
        return tacc_tok_iter_peek_nomacro(iter);
    }
    if (tok->kind == TOK_FAKE_PMARK) {
        tacc_token_list_pop(iter->pending);
        tacc_pp_tok_free(tok);

        /* simple recursion, this shouldn't go too deep */
        return tacc_tok_iter_peek_nomacro(iter);
    }
    return tok;
}

/* return: owning, iter: borrow */
static struct pp_tok *
tacc_tok_iter_consume_nomacro(struct tacc_tok_iter *iter) {
    struct pp_tok *tok;

    tok = tacc_tok_iter_peek_nomacro(iter);
    tacc_token_list_pop(iter->pending);

    return tok;
}

/* iter: borrow */
static void tacc_tok_iter_drop_nomacro(struct tacc_tok_iter *iter) {
    tacc_pp_tok_free(tacc_tok_iter_consume_nomacro(iter));
}

/* iter: borrow, tok: owning */
static void tacc_tok_iter_push_pending(struct tacc_tok_iter *iter,
                                       struct pp_tok *tok) {
    tacc_token_list_push(iter->pending, tok);
}

/* iter: borrow, tok: borrowing */
static void tacc_tok_iter_join_pending(struct tacc_tok_iter *iter,
                                       struct pp_tok *tok) {
    struct tacc_token_list_entry *tok_entry;
    struct tacc_file_iter *file_iter;
    struct pp_tok *old_tok;
    struct pp_tok *new_tok;
    struct tacc_string *new_tok_str;
    char *new_tok_str_c;
    size_t new_tok_size;

    tok_entry = tacc_token_list_get(iter->pending,
                                    tacc_token_list_len(iter->pending) - 1);
    old_tok = tok_entry->content;
    new_tok_str = tacc_dynstring_new();

    /*
     * pending is pushed in right-to-left order.
     * Later tokens will already be present in the stack, so we join in reverse
     * order.
     */
    tacc_dynstring_join(new_tok_str, tok->str);
    tacc_dynstring_join(new_tok_str, old_tok->str);
    tacc_pp_tok_free(old_tok);

    new_tok_size = tacc_dynstring_len(new_tok_str);
    new_tok_str_c = tacc_dynstring_take_str(new_tok_str);
    tacc_dynstring_free(new_tok_str);
    file_iter =
        tacc_file_iter_new_str(new_tok_str_c, new_tok_str_c + new_tok_size);

    new_tok = tacc_file_iter_lex(file_iter, LEX_IN_REPLACEMENT_LIST);
    new_tok->preceded_by_ws = tok->preceded_by_ws;
    tacc_file_iter_eat_ws_no_newlines(file_iter);
    tacc_assert(tacc_file_is_eof(file_iter),
                "multiple tokens produced by joining: %s",
                new_tok_str);

    tacc_file_iter_free(file_iter);

    tok_entry->content = new_tok;
}

/* iter: borrow, macro_name: borrow */
static void tacc_tok_iter_insert_macro_replacing_stop(
    struct tacc_tok_iter *iter, char *macro_name) {
    struct pp_tok *tok;

    tok = tacc_pp_tok_new();
    tok->kind = TOK_FAKE_END_OF_MACRO;
    tacc_dynstring_concat(tok->str, macro_name);
    tacc_tok_iter_push_pending(iter, tok);
}

/* iter: borrow */
static void tacc_tok_iter_push_placemarker(struct tacc_tok_iter *iter) {
    struct pp_tok *tok;

    tok = tacc_pp_tok_new();
    tok->kind = TOK_FAKE_PMARK;
    tacc_tok_iter_push_pending(iter, tok);
}

/* macro_def: borrow, ident: borrow */
static size_t tacc_macro_def_index_of_par(struct tacc_macro_def *macro_def,
                                          char *ident) {
    size_t i;
    struct tacc_string_list_entry *par_ident;

    if (macro_def->is_va && (!strcmp(ident, "__VA_ARGS__"))) {
        return tacc_string_list_len(macro_def->params);
    }
    for (i = 0; i < tacc_string_list_len(macro_def->params); ++i) {
        par_ident = tacc_string_list_get(macro_def->params, i);
        if (!strcmp(tacc_dynstring_as_str(par_ident->content), ident)) {
            return i;
        }
    }

    return TACC_PARAM_NOT_FOUND;
}

/* return: owning, macro_def: borrow, raw_args: owning */
static struct tacc_token_list *tacc_pp_split_args(
    struct tacc_macro_def *macro_def, struct tacc_token_list *raw_args) {
    struct tacc_token_list *ret;
    struct tacc_token_list *ret_cur;
    struct pp_tok *raw_arg;
    struct tacc_token_list_entry *raw_arg_entry;
    size_t nest_level;
    size_t i;
    size_t j;
    size_t max_param;

    nest_level = 0;
    tacc_assert(macro_def->is_function_like,
                "object-like macro takes no params");
    max_param = tacc_string_list_len(macro_def->params);
    /* +1 to leave space for overflow, even if varargs are not used */
    ret = tacc_malloc((max_param + 1) * sizeof(struct tacc_token_list));
    j = 0;
    if (macro_def->is_va) {
        max_param = max_param + 1;
    }
    for (i = 0; i < max_param; i = i + 1) {
        ret_cur = ret + tacc_sizeadj(i, sizeof(struct tacc_token_list));
        tacc_token_list_init(ret_cur);
        while (j < tacc_token_list_len(raw_args)) {
            raw_arg_entry = tacc_token_list_get(raw_args, j);
            raw_arg = raw_arg_entry->content;

            if ((nest_level == 0) && (raw_arg->kind == TOK_COMMA) &&
                (i < tacc_string_list_len(macro_def->params))) {
                /* if i == tacc_macro_def_num_params, we are looking at va_args
                 */
                j = j + 1;

                tacc_pp_tok_free(raw_arg);

                break;
            }
            if (raw_arg->kind == TOK_LPAREN) {
                nest_level = nest_level + 1;
            }
            if (raw_arg->kind == TOK_RPAREN) {
                nest_level = nest_level - 1;
            }
            j = j + 1;
            tacc_token_list_push(ret_cur, raw_arg);
        }
        tacc_assert(nest_level == 0, "missing ) in macro argument list");
    }

    tacc_dynarray_free(raw_args->list);
    tacc_free(raw_args);

    return ret;
}

/* return: owning, tokens: borrowing */
static struct pp_tok *tacc_pp_stringify(struct tacc_token_list *tokens) {
    struct pp_tok *ret;
    struct tacc_string *ret_tok_str;
    struct tacc_string *this_tok_str;
    struct tacc_token_list_entry *tok_entry;
    struct pp_tok *tok;
    size_t i;
    size_t j;
    char ch;

    ret_tok_str = tacc_dynstring_new();
    ret = tacc_pp_tok_new();
    ret->kind = TOK_STRING;

    tacc_dynstring_push(ret_tok_str, '"');
    for (i = 0; i < tacc_token_list_len(tokens); i = i + 1) {
        tok_entry = tacc_token_list_get(tokens, i);
        tok = tok_entry->content;
        if ((i != 0) && tok->preceded_by_ws) {
            tacc_dynstring_push(ret_tok_str, ' ');
        }
        if ((tok->kind == TOK_STRING) || (tok->kind == TOK_CHAR)) {
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
    }
    tacc_dynstring_push(ret_tok_str, '"');
    tacc_pp_tok_assign_tstr(ret, ret_tok_str);

    return ret;
}

/* iter: borrow, tokens: borrow, macro_def: borrow */
static void tacc_tok_iter_push_all_expanding(struct tacc_tok_iter *iter,
                                             struct tacc_token_list *tokens,
                                             size_t start_at,
                                             size_t num_tokens,
                                             tacc_bool first_preceded_by_ws,
                                             struct tacc_macro_def *macro_def) {
    struct tacc_tok_iter *helper_iter;
    struct pp_tok *tok;
    struct tacc_token_list_entry *token_cur;
    struct tacc_token_list *reverse_toks;
    size_t i;
    size_t tok_count;

    helper_iter = tacc_tok_iter_new(NULL, iter->state);

    /* Soak the sponge */
    for (i = 0; i < num_tokens; i = i + 1) {
        token_cur = tacc_token_list_get(tokens, start_at + num_tokens - i - 1);
        tacc_tok_iter_push_pending(helper_iter,
                                   tacc_pp_tok_clone(token_cur->content));
    }

    /* Squeeze the sponge */
    macro_def->is_replacing = 0;
    reverse_toks = tacc_token_list_new();
    tok_count = 0;
    while (1) {
        tok = tacc_tok_iter_next(helper_iter);
        if (tok->kind == TOK_EOF) {
            tacc_pp_tok_free(tok);
            break;
        }
        if ((tok_count == 0) && (tok->preceded_by_ws != first_preceded_by_ws)) {
            tok->preceded_by_ws = first_preceded_by_ws;
        }
        tacc_token_list_push(reverse_toks, tok);
        tok_count = tok_count + 1;
    }
    /* Reverse order, for buffer */
    for (i = 0; i < tok_count; i = i + 1) {
        token_cur = tacc_token_list_get(reverse_toks,
                                        tacc_token_list_len(reverse_toks) - 1);
        tacc_token_list_pop(reverse_toks);
        tacc_tok_iter_push_pending(iter, token_cur->content);
    }
    tacc_token_list_free(reverse_toks);
    tacc_free(reverse_toks);
    tacc_tok_iter_free(helper_iter);
    macro_def->is_replacing = 1;
}

/* iter_within: borrow, macro_def: borrow, arg_list: owned */
static void tacc_pp_macro_def_func_expand(struct tacc_tok_iter *iter_within,
                                          struct tacc_macro_def *macro_def,
                                          struct tacc_token_list *arg_list) {
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

    struct pp_tok *replacing_tok;
    struct tacc_token_list_entry *replacing_tok_entry;
    struct pp_tok *next_tok;
    struct tacc_token_list_entry *next_tok_entry;
    size_t i;
    size_t max_param;
    size_t par_position;
    struct tacc_token_list *split_args;
    struct tacc_token_list *arg_entry;
    struct tacc_token_list_entry *arg_last_tok;

    split_args = tacc_pp_split_args(macro_def, arg_list);
    arg_list = NULL;

    for (i = 0; i < tacc_token_list_len(macro_def->replacement_list);
         i = i + 1) {
        replacing_tok_entry = tacc_token_list_get(
            macro_def->replacement_list,
            tacc_token_list_len(macro_def->replacement_list) - i - 1);
        replacing_tok = replacing_tok_entry->content;
        tacc_assert(replacing_tok->kind != TOK_SHARP,
                    "stray # in function-like macro expansion list");
        if (replacing_tok->kind == TOK_SHARP_2) {
            i = i + 1;
            tacc_assert(i < tacc_token_list_len(macro_def->replacement_list),
                        "## on edge of function-like macro expansion list");

            replacing_tok_entry = tacc_token_list_get(
                macro_def->replacement_list,
                tacc_token_list_len(macro_def->replacement_list) - i - 1);
            next_tok = replacing_tok_entry->content;
            if (next_tok->kind != TOK_IDENT) {
                /* borrows next_tok */
                tacc_tok_iter_join_pending(iter_within, next_tok);
                continue;
            }

            par_position = tacc_macro_def_index_of_par(
                macro_def, tacc_dynstring_as_str(next_tok->str));
            if (par_position == TACC_PARAM_NOT_FOUND) {
                /* borrows next_tok */
                tacc_tok_iter_join_pending(iter_within, next_tok);
                continue;
            }
            arg_entry =
                split_args +
                tacc_sizeadj(par_position, sizeof(struct tacc_token_list));
            if (tacc_token_list_len(arg_entry) == 0) {
                /*
                 * There must be a previous token (which is possibly a
                 * placemarker, or a real token). Do not push a placemarker.
                 */
                continue;
            }
            arg_last_tok = tacc_token_list_get(
                arg_entry, tacc_token_list_len(arg_entry) - 1);
            tacc_tok_iter_join_pending(iter_within, arg_last_tok->content);
            if (tacc_token_list_len(arg_entry) > 1) {
                tacc_tok_iter_push_all_expanding(
                    iter_within,
                    arg_entry,
                    0,
                    tacc_token_list_len(arg_entry) - 1,
                    next_tok->preceded_by_ws,
                    macro_def);
            }
            continue;
        }
        if (replacing_tok->kind != TOK_IDENT) {
            tacc_tok_iter_push_pending(iter_within,
                                       tacc_pp_tok_clone(replacing_tok));
            continue;
        }
        par_position = tacc_macro_def_index_of_par(
            macro_def, tacc_dynstring_as_str(replacing_tok->str));
        if (par_position == TACC_PARAM_NOT_FOUND) {
            tacc_tok_iter_push_pending(iter_within,
                                       tacc_pp_tok_clone(replacing_tok));
            continue;
        }
        arg_entry = split_args +
                    tacc_sizeadj(par_position, sizeof(struct tacc_token_list));
        if (i == tacc_token_list_len(macro_def->replacement_list) - 1) {
            if (tacc_token_list_len(arg_entry) == 0) {
                /* nothing to do */
                continue;
            }
            tacc_tok_iter_push_all_expanding(iter_within,
                                             arg_entry,
                                             0,
                                             tacc_token_list_len(arg_entry),
                                             0,
                                             macro_def);
            continue;
        }
        next_tok_entry = tacc_token_list_get(
            macro_def->replacement_list,
            tacc_token_list_len(macro_def->replacement_list) - i - 2);
        next_tok = next_tok_entry->content;
        if (next_tok->kind == TOK_SHARP_2) {
            if (tacc_token_list_len(arg_entry) == 0) {
                tacc_tok_iter_push_placemarker(iter_within);
                continue;
            }
            tacc_tok_iter_push_all_expanding(iter_within,
                                             arg_entry,
                                             1,
                                             tacc_token_list_len(arg_entry) - 1,
                                             replacing_tok->preceded_by_ws,
                                             macro_def);
            next_tok_entry = tacc_token_list_get(arg_entry, 0);
            tacc_tok_iter_push_pending(
                iter_within, tacc_pp_tok_clone(next_tok_entry->content));
            continue;
        }
        if (next_tok->kind == TOK_SHARP) {
            /* borrows arg */
            next_tok = tacc_pp_stringify(arg_entry);
            if (iter_within->in_include_directive) {
                next_tok->kind = TOK_INCDIR_STRING;
            }
            tacc_tok_iter_push_pending(iter_within, next_tok);

            /* consume */
            i = i + 1;
            continue;
        }
        if (tacc_token_list_len(arg_entry) == 0) {
            /* nothing to do */
            continue;
        }
        tacc_tok_iter_push_all_expanding(iter_within,
                                         arg_entry,
                                         0,
                                         tacc_token_list_len(arg_entry),
                                         replacing_tok->preceded_by_ws,
                                         macro_def);
    }

    max_param = tacc_string_list_len(macro_def->params);
    if (macro_def->is_va) {
        max_param = max_param + 1;
    }
    for (i = 0; i < max_param; i = i + 1) {
        arg_entry =
            split_args + tacc_sizeadj(i, sizeof(struct tacc_token_list));
        tacc_token_list_free(arg_entry);
    }
    tacc_free(split_args);
}

static struct pp_tok *tacc_tok_iter_eval_defined(struct tacc_tok_iter *iter) {
    struct pp_tok *tok;
    tacc_bool had_paren;
    tacc_bool is_defined;

    tacc_tok_iter_drop_nomacro(iter); /* drop "defined" */
    tok = tacc_tok_iter_consume_nomacro(iter);
    if (tok->kind == TOK_LPAREN) {
        had_paren = 1;
        tacc_pp_tok_free(tok);
        tok = tacc_tok_iter_consume_nomacro(iter);
    } else {
        had_paren = 0;
    }
    tacc_assert(tok->kind == TOK_IDENT, "expected identified after defined");
    is_defined = tacc_pp_macro_is_defined(iter->state, tok->str->string);
    tacc_pp_tok_free(tok);
    tok = NULL;

    if (had_paren) {
        tok = tacc_tok_iter_consume_nomacro(iter);
        tacc_assert(tok->kind == TOK_RPAREN, "expected ) to close defined(");
        tacc_pp_tok_free(tok);
        tok = NULL;
    }

    tok = tacc_pp_tok_new();
    tok->kind = TOK_PPNUM;
    tok->is_final = 1;
    if (is_defined) {
        tacc_dynstring_concat(tok->str, "1");
    } else {
        tacc_dynstring_concat(tok->str, "0");
    }
    tacc_tok_iter_push_pending(iter, tok);

    return tok;
}

static struct pp_tok *tacc_tok_iter_push_0(struct tacc_tok_iter *iter) {
    struct pp_tok *tok;

    tok = tacc_pp_tok_new();
    tok->kind = TOK_PPNUM;
    tok->is_final = 1;
    tacc_dynstring_concat(tok->str, "0");

    tacc_tok_iter_push_pending(iter, tok);

    return tok;
}

static struct pp_tok *tacc_tok_iter_maybe_nonmacro_ident(
    struct tacc_tok_iter *iter, struct pp_tok *tok) {
    if (!iter->in_if) {
        return tok;
    }
    tok = NULL;
    tacc_tok_iter_drop_nomacro(iter);
    return tacc_tok_iter_push_0(iter);
}

/* return: borrowing, iter: borrowing */
static struct pp_tok *
tacc_tok_iter_peek_handle_macros(struct tacc_tok_iter *iter) {
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
    struct pp_tok* tok;
    struct pp_tok* new_tok;
    struct tacc_token_list_entry * src_tok_entry;
    struct tacc_token_list * arg_list;
    struct tacc_macro_def_list_entry* macro_entry;
    struct tacc_macro_def* macro_def;
    size_t i;
    size_t nest_level;

    while (1) {
        tok = tacc_tok_iter_peek_nomacro(iter);
        if (tok->kind != TOK_IDENT) {
            return tok;
        }
        if (tok->is_final) {
            return tacc_tok_iter_maybe_nonmacro_ident(iter, tok);
        }
        if (iter->in_if && !strcmp(tok->str->string, "defined")) {
            return tacc_tok_iter_eval_defined(iter);
        }

        macro_entry = tacc_pp_find_macro_or_first_empty(
            iter->state, tacc_dynstring_as_str(tok->str));
        macro_def = macro_entry->content;
        if (!macro_def) {
            return tacc_tok_iter_maybe_nonmacro_ident(iter, tok);
        }
        if (macro_def->is_tombstone) {
            return tacc_tok_iter_maybe_nonmacro_ident(iter, tok);
        }

        tacc_token_list_pop(iter->pending);
        /* tok is now owning */

        if (!macro_def->is_function_like) {
            macro_def->is_replacing = 1;

            tacc_pp_tok_free(tok);
            tok = NULL;

            tacc_tok_iter_insert_macro_replacing_stop(
                iter, tacc_dynstring_as_str(macro_def->name));
            for (i = 0; i < tacc_token_list_len(macro_def->replacement_list);
                 i = i + 1) {
                src_tok_entry =
                    tacc_token_list_get(macro_def->replacement_list, tacc_token_list_len(macro_def->replacement_list) - i - 1);

                if (src_tok_entry->content->kind ==
                    TOK_SHARP_2) {
                    tacc_assert(i != 0,
                                "unexpected ## at beginning of replacement "
                                "list of object-like macro");
                    tacc_assert(
                        i != tacc_token_list_len(macro_def->replacement_list) - 1,
                        "unexpected ## at end of replacement list of "
                        "object-like macro");

                    i = i + 1;
                    src_tok_entry =
                        tacc_token_list_get(macro_def->replacement_list, tacc_token_list_len(macro_def->replacement_list) - i - 1);
                    /* borrows src_tok_entry->content */
                    tacc_tok_iter_join_pending(
                        iter, src_tok_entry->content);
                } else {
                    tacc_tok_iter_push_pending(
                        iter, tacc_pp_tok_clone(src_tok_entry->content));
                }
            }
            continue;
        }
        new_tok = tacc_tok_iter_peek_nomacro(iter);
        if (new_tok->kind != TOK_LPAREN) {
            /*
             * Not matched as a function-like macro; push `tok` back into queue.
             * To prevent an infinite loop, immediately return `tok`.
             * Also, give ownership of tok back.
             */
            tacc_tok_iter_push_pending(iter, tok);
            return tacc_tok_iter_maybe_nonmacro_ident(iter, tok);
        }

        /* consume new_tok (lparen) and macro name token (tok) */
        tacc_tok_iter_drop_nomacro(iter);
        tacc_pp_tok_free(tok);
        tok = NULL;

        arg_list = tacc_token_list_new();
        nest_level = 1;
        i = 0;
        while (1) {
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
                if (nest_level == 0) {
                    tacc_pp_tok_free(tok);
                    break;
                }
            }
            tacc_token_list_push(arg_list, tok);
        }

        tacc_tok_iter_insert_macro_replacing_stop(
            iter, tacc_dynstring_as_str(macro_def->name));
        macro_def->is_replacing = 1;

        tacc_pp_macro_def_func_expand(iter, macro_def, arg_list);
    }
}

void tacc_tok_iter_free(struct tacc_tok_iter *iter) {
    struct tacc_token_list_entry *tok_entry;

	if (iter->file_iter) {
        tacc_file_iter_free(iter->file_iter);
	}
    if (iter->override != NULL) {
        tacc_tok_iter_free(iter->override);
    }

    tacc_assert((iter->skip_level == 0) && (iter->inc_level == 0), "unclosed conditional inclusion");

    if (tacc_token_list_len(iter->pending) > 0) {
        tacc_assert(tacc_token_list_len(iter->pending) == 1, "tokens left pending in iterator");
        tok_entry = tacc_token_list_get(iter->pending, 0);
        tacc_assert(tok_entry->content->kind == TOK_EOF, "tokens left pending in iterator");
        /* will be freed by tacc_token_list_free */
    }
    tacc_token_list_free(iter->pending);
    tacc_free(iter->pending);
    tacc_free(iter);
}

/* return: borrow, first: borrow */
static struct pp_tok* tacc_tok_iter_peek_handle_directives(struct tacc_tok_iter* first) {
    struct tacc_tok_iter* last_iter;
    struct tacc_tok_iter* prev_iter;
    struct pp_tok* peek_tok;
    struct tacc_string *directive;

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
            if (last_iter->file_iter) {
                printf("hit eof in %s\n", last_iter->file_iter->filename);
            } else {
                printf("hit eof in unnamed file\n");
            }
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
            /* frees peek_tok */
            tacc_tok_iter_free(last_iter);
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
        peek_tok = tacc_tok_iter_consume_nomacro(last_iter);
        directive = peek_tok->str;
        peek_tok->str = tacc_dynstring_new();
        tacc_pp_tok_free(peek_tok);
        peek_tok = NULL;

        /* Handle directive. This will never skip over any tokens. */
        tacc_tok_iter_handle_directive(first, directive);
    }
}

/* return: borrow, iter: borrow */
struct pp_tok* tacc_tok_iter_peek(struct tacc_tok_iter* iter) {
    return tacc_tok_iter_peek_handle_directives(iter);
}

/* return: owning, iter: borrow */
struct pp_tok* tacc_tok_iter_next(struct tacc_tok_iter* iter) {
    struct tacc_tok_iter* level;
    struct pp_tok* ret;

    ret = tacc_tok_iter_peek(iter);

    level = tacc_tok_iter_cur_iter(iter);
    tacc_token_list_pop(level->pending);

    return ret;
}

tacc_bool tacc_tok_iter_accept_tok(struct tacc_tok_iter *iter,
                                   enum pp_tok_kind tok) {
    struct pp_tok* peek_tok;

    peek_tok = tacc_tok_iter_peek(iter);
    if (peek_tok->kind == tok) {
        tacc_pp_tok_free(tacc_tok_iter_next(iter));
        return 1;
    }
    return 0;
}
void tacc_tok_iter_deaccept_tok(struct tacc_tok_iter *iter,
                                enum pp_tok_kind tok) {
    struct pp_tok* new_tok;

    new_tok = tacc_pp_tok_new();
    tacc_pp_tok_init(new_tok);
    new_tok->kind = tok;
    new_tok->is_final = 1;
    tacc_tok_iter_push_pending(iter, new_tok);
}
tacc_bool tacc_tok_iter_accept_kw(struct tacc_tok_iter *iter,
                                  enum pp_ident_kind kw) {
    struct pp_tok* peek_tok;

    peek_tok = tacc_tok_iter_peek(iter);
    if (peek_tok->kind != TOK_IDENT) {
        return 0;
    }
    if (peek_tok->ident_kind != kw) {
        return 0;
    }
    tacc_pp_tok_free(tacc_tok_iter_next(iter));
    return 1;
}


void tacc_pp_state_free(struct tacc_pp_state *state) {
    tacc_string_list_free(state->include_path);
    tacc_free(state->include_path);
    tacc_macro_def_list_free(state->macros);
    tacc_free(state->macros);
    tacc_free(state);
}

#include "tasku_pp.h"
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
    iter->tacc_file_iter_is_bol = 1;
    iter->tacc_file_iter_is_ws = 1;
    iter->tacc_file_iter_src = start;
    iter->tacc_file_iter_end = end;
    iter->tacc_file_iter_filename = NULL;
}

void tacc_file_iter_init(tacc_file_iter_p iter, tacc_file_p file) {
    tacc_file_iter_init_str(
        iter, file->tacc_file_src, file->tacc_file_src + file->tacc_file_len);
    iter->tacc_file_iter_filename = file->tacc_file_name;
}

char *tacc_pp_to_string(pp_tok_p tok) {
    switch (tok->pp_tok__kind) {
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
    case TOK_FAKE_END_OF_MACRO:
        return "TOK_FAKE_END_OF_MACRO";
    case TOK_FAKE_PMARK:
        return "TOK_FAKE_PMARK";
    }
    return "UNRECOGNIZED";
}

static void tacc_file_iter_eat_splices(tacc_file_iter_p iter) {
    char ch;
    ch = *iter->tacc_file_iter_src;

    if (ch == '\\') {
        /* c99 5.1.1.2p2: a physical source file cannot end with a backslash */
        ch = iter->tacc_file_iter_src[1];
        if (ch == '\n') {
            iter->tacc_file_iter_src = iter->tacc_file_iter_src + 1;
            iter->tacc_file_iter_src = iter->tacc_file_iter_src + 1;
        }
    }
}

static char *tacc_file_iter_cur(tacc_file_iter_p iter) {
    tacc_file_iter_eat_splices(iter);
    return iter->tacc_file_iter_src;
}

static int tacc_file_is_eof(tacc_file_iter_p iter) {
    if (tacc_file_iter_cur(iter) == iter->tacc_file_iter_end) {
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
        iter->tacc_file_iter_is_bol = 1;
    } else {
        iter->tacc_file_iter_is_bol = 0;
    }
    if (ch == '\n' || ch == 9 || ch == ' ') {
        iter->tacc_file_iter_is_ws = 1;
    } else {
        iter->tacc_file_iter_is_ws = 0;
    }

    iter->tacc_file_iter_src = iter->tacc_file_iter_src + 1;
    return ch;
}

static int tacc_file_iter_accept_ch(tacc_file_iter_p iter, char accept) {
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
    iter->tacc_file_iter_is_ws = 1;
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
    iter->tacc_file_iter_is_ws = 1;
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
        if ((iter->tacc_file_iter_end - iter->tacc_file_iter_src) >= 2) {
            ch = *iter->tacc_file_iter_src;
            if (ch == '/') {
                ch = iter->tacc_file_iter_src[1];
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
        if ((iter->tacc_file_iter_end - iter->tacc_file_iter_src) >= 2) {
            ch = *iter->tacc_file_iter_src;
            if (ch == '/') {
                ch = iter->tacc_file_iter_src[1];
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

static char *tacc_file_iter_lex_escape(tacc_file_iter_p iter,
                                       char *out_str,
                                       char *out_str_end) {
    char ch;
    char *ret;
    ret = out_str;

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
        tacc_assert((out_str_end - out_str) >= 3, "overlong escape");
        *ret = '\\';
        ret = ret + 1;
        *ret = ch;
        ret = ret + 1;
        return ret;
    default:
        tacc_assert((out_str_end - out_str) >= 5, "overlong escape");
        tacc_assert(ch >= '0' && ch <= '7', "invalid escape %x", ch);
        *ret = '\\';
        ret = ret + 1;
        *ret = ch;
        ret = ret + 1;

        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '7') {
            *ret = ch;
            ret = ret + 1;
        } else {
            return ret;
        }
        tacc_file_iter_consume_ch(iter);
        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '7') {
            tacc_file_iter_consume_ch(iter);
            *ret = ch;
            ret = ret + 1;
        }
        return ret;
    }
}

static int tacc_file_iter_lex_directive(tacc_file_iter_p iter,
                                        pp_tok_p tok_out) {
    char *directive_str;
    char *directive_str_end;
    int was_bol, was_ws;

    was_bol = iter->tacc_file_iter_is_bol;
    tacc_file_iter_eat_ws_no_newlines(iter);
    if (!was_bol && !tacc_file_iter_accept_ch(iter, '\n')) {
        return 0;
    }
    tacc_file_iter_eat_all_ws(iter);
    if (!tacc_file_iter_accept_ch(iter, '#')) {
        return 0;
    }

    directive_str = tacc_malloc(4096);
    directive_str[4095] = 0;
    directive_str_end = directive_str + 4095;
    tok_out->pp_tok_str = directive_str;

    /* file must end in newline, no need to check for eof */
    was_ws = 0;
    while (!tacc_file_iter_accept_ch(iter, '\n')) {
        tacc_assert(directive_str != directive_str_end, "overlong directive");

        if (!was_ws) {
            tacc_file_iter_eat_ws_no_newlines(iter);
            if (iter->tacc_file_iter_is_ws) {
                /*
                 * 5.1.1.2p3 permits replacing any nonempty whitespace sequence
                 * with a single space This simplifies comment handling.
                 */
                *directive_str = ' ';
                directive_str = directive_str + 1;
                was_ws = 1;
                continue;
            }
        }
        *directive_str = tacc_file_iter_consume_ch(iter);
        directive_str = directive_str + 1;
        was_ws = 0;
    }

    tok_out->pp_tok__kind = TOK_DIRECTIVE;
    *directive_str = 0;
    tok_out->pp_tok_end = directive_str;

    return 1;
}

static pp_tok_p tacc_file_iter_lex_char(tacc_file_iter_p iter,
                                        pp_tok_p tok_out) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    pp_tok_p ret = tok_out;
    int contained;
    char *out_str;

    ret->pp_tok__kind = TOK_CHAR;
    out_str = tacc_malloc(8);
    out_str[0] = '\'';
    ret->pp_tok_str = out_str;
    ret->pp_tok_end = out_str + 7;

    tacc_assert(!tacc_file_iter_accept_ch(iter, '\''),
                "empty character literal");
    if (!tacc_file_iter_accept_ch(iter, '\\')) {
        contained = tacc_file_iter_consume_ch(iter);
        tacc_assert(tacc_file_iter_accept_ch(iter, '\''),
                    "overlong character literal");

        *out_str = (char) contained;
        out_str = out_str + 1;
        *out_str = '\'';
        out_str = out_str + 1;
        *out_str = 0;
        ret->pp_tok_end = out_str;
        return ret;
    }
    out_str = tacc_file_iter_lex_escape(iter, out_str, ret->pp_tok_end);
    tacc_assert(tacc_file_iter_accept_ch(iter, '\''),
                "overlong character literal");
    *out_str = '\\';
    out_str = out_str + 1;
    *out_str = 0;
    ret->pp_tok_end = out_str;

    return ret;
}

static pp_tok_p tacc_file_iter_lex_string(tacc_file_iter_p iter,
                                          pp_tok_p tok_out) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    pp_tok_p ret = tok_out;
    char *out_str;

    ret->pp_tok__kind = TOK_STRING;

    /* maybe make dynamic later */
    out_str = tacc_malloc(4096);
    out_str[4095] = 0;

    ret->pp_tok_str = out_str;
    ret->pp_tok_end = out_str + 4095;

    while (!tacc_file_iter_accept_ch(iter, '"')) {
        tacc_assert(!tacc_file_iter_accept_ch(iter, '\n'),
                    "newline in string literal");
        tacc_assert(out_str != ret->pp_tok_end, "overlong string literal");
        if (!tacc_file_iter_accept_ch(iter, '\\')) {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        out_str = tacc_file_iter_lex_escape(iter, out_str, ret->pp_tok_end);
    }
    *out_str = 0;
    ret->pp_tok_end = out_str;

    return ret;
}

static pp_tok_p tacc_file_iter_lex_ppnum(tacc_file_iter_p iter,
                                         pp_tok_p tok_out,
                                         char first) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    pp_tok_p ret = tok_out;
    char *out_str;
    char ch;

    ret->pp_tok__kind = TOK_PPNUM;

    /* maybe make dynamic later */
    out_str = tacc_malloc(128);
    out_str[127] = 0;

    ret->pp_tok_str = out_str;
    ret->pp_tok_end = out_str + 127;

    *out_str = first;
    out_str = out_str + 1;

    while (1) {
        tacc_assert(out_str != ret->pp_tok_end, "overlong ppnumber literal");
        if (tacc_file_is_eof(iter)) {
            break;
        }
        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '9') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        if (ch == 'p' || ch == 'P' || ch == 'e' || ch == 'E') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            if (tacc_file_iter_accept_ch(iter, '+')) {
                *out_str = '+';
                out_str = out_str + 1;
                continue;
            }
            if (tacc_file_iter_accept_ch(iter, '-')) {
                *out_str = '-';
                out_str = out_str + 1;
                continue;
            }
            continue;
        }
        if (ch >= 'a' && ch <= 'z') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        if (ch >= 'A' && ch <= 'Z') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        if (ch == '_' || ch == '.') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        break;
    }
    *out_str = 0;
    ret->pp_tok_end = out_str;

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
    char *out_str;
    char ch;

    ret->pp_tok__kind = TOK_IDENT;

    /* maybe make dynamic later */
    out_str = tacc_malloc(128);
    out_str[127] = 0;

    ret->pp_tok_str = out_str;
    ret->pp_tok_end = out_str + 127;

    *out_str = first;
    out_str = out_str + 1;

    while (1) {
        tacc_assert(out_str != ret->pp_tok_end, "overlong ident");
        if (tacc_file_is_eof(iter)) {
            break;
        }
        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '9') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        if (ch >= 'a' && ch <= 'z') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        if (ch >= 'A' && ch <= 'Z') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        if (ch == '_') {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        break;
    }
    *out_str = 0;
    ret->pp_tok_end = out_str;

    ret->pp_tok_ident_kind = tacc_recognize_ident_kind(ret->pp_tok_str);

    return ret;
}

static void tacc_pp_tok_assign_str(pp_tok_p tok, char *str) {
    tok->pp_tok_str = str;
    tok->pp_tok_end = str + strlen(str);
}

static int tacc_file_iter_maybe_special(tacc_file_iter_p iter,
                                        pp_tok_p tok_out,
                                        pp_tok_kind_e special_tok,
                                        char *special_match) {
    if (!tacc_file_iter_accept_ch(iter, special_match[1])) {
        tok_out->pp_tok_str = tacc_malloc(2);
        tok_out->pp_tok_str[0] = special_match[0];
        tok_out->pp_tok_str[1] = 0;
        tok_out->pp_tok_end = tok_out->pp_tok_str + 1;
        return 0;
    }
    tok_out->pp_tok__kind = special_tok;
    tacc_pp_tok_assign_str(tok_out, special_match);
    return 1;
}

static pp_tok_p tacc_file_iter_lex(tacc_file_iter_p iter,
                                   int have_directives,
                                   int only_directives) {
    char first;
    char ch;
    pp_tok_p ret;
    pp_tok_kind_e kind;

    ret = tacc_malloc(sizeof(struct pp_tok));
    ret->pp_tok_is_final = 0;

    if (only_directives) {
        while (1) {
            if (tacc_file_is_eof(iter)) {
                ret->pp_tok__kind = TOK_EOF;
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
        ret->pp_tok__kind = TOK_EOF;
        tacc_pp_tok_assign_str(ret, "");
        return ret;
    }

    if (have_directives) {
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
    ret->pp_tok_preceded_by_ws = iter->tacc_file_iter_is_ws;

    if (tacc_file_is_eof(iter)) {
        ret->pp_tok__kind = TOK_EOF;
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
        tacc_assert(!have_directives, "stray # outside directive");
        kind = TOK_SHARP;
        if (tacc_file_iter_maybe_special(iter, ret, TOK_SHARP_2, "##")) {
            return ret;
        }
        break;
    case '\'':
        return tacc_file_iter_lex_char(iter, ret);
    case '"':
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
            ((iter->tacc_file_iter_end - iter->tacc_file_iter_src) >= 2) &&
            iter->tacc_file_iter_src[1] == '.') {
            ret->pp_tok__kind = TOK_DOT_3;
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
        ret->pp_tok__kind = TOK_MINUS;
        tacc_file_iter_maybe_special(iter, ret, TOK_MINUS_EQ, "-=");
        return ret;
    case '&':
        if (tacc_file_iter_maybe_special(iter, ret, TOK_AMPERSAND_2, "&&")) {
            return ret;
        }
        ret->pp_tok__kind = TOK_AMPERSAND;
        tacc_file_iter_maybe_special(iter, ret, TOK_AMPERSAND_EQ, "&=");
        return ret;
    case '*':
        ret->pp_tok__kind = TOK_ASTERISK;
        tacc_file_iter_maybe_special(iter, ret, TOK_ASTERISK_EQ, "*=");
        return ret;
    case '+':
        if (tacc_file_iter_maybe_special(iter, ret, TOK_PLUS_2, "++")) {
            return ret;
        }
        ret->pp_tok__kind = TOK_PLUS;
        tacc_file_iter_maybe_special(iter, ret, TOK_PLUS_EQ, "+=");
        return ret;
    case '~':
        kind = TOK_TILDE;
        break;
    case '!':
        ret->pp_tok__kind = TOK_EXCLAMATION;
        tacc_file_iter_maybe_special(iter, ret, TOK_EXCLAMATION_EQ, "!=");
        return ret;
    case '^':
        ret->pp_tok__kind = TOK_CIRCUMFLEX;
        tacc_file_iter_maybe_special(iter, ret, TOK_CIRCUMFLEX_EQ, "^=");
        return ret;
    case '|':
        if (tacc_file_iter_maybe_special(iter, ret, TOK_PIPE_EQ, "|=")) {
            return ret;
        }
        ret->pp_tok__kind = TOK_PIPE;
        tacc_file_iter_maybe_special(iter, ret, TOK_PIPE_2, "||");
        return ret;
    case '/':
        ret->pp_tok__kind = TOK_SLASH;
        tacc_file_iter_maybe_special(iter, ret, TOK_SLASH_EQ, "/=");
        return ret;
    case '%':
        ret->pp_tok__kind = TOK_PERCENT;
        tacc_file_iter_maybe_special(iter, ret, TOK_PERCENT_EQ, "%=");
        return ret;
    case '<':
        kind = TOK_LT;
        if (tacc_file_iter_maybe_special(iter, ret, TOK_LT_EQ, "<=")) {
            return ret;
        }
        if (tacc_file_iter_accept_ch(iter, '<')) {
            if (tacc_file_iter_accept_ch(iter, '=')) {
                ret->pp_tok__kind = TOK_LT_2_EQ;
                tacc_pp_tok_assign_str(ret, "<<=");
                return ret;
            } else {
                ret->pp_tok__kind = TOK_LT_2;
                tacc_pp_tok_assign_str(ret, "<<");
                return ret;
            }
        }
        break;
    case '>':
        kind = TOK_GT;
        if (tacc_file_iter_maybe_special(iter, ret, TOK_GT_EQ, "<=")) {
            return ret;
        }
        if (tacc_file_iter_accept_ch(iter, '>')) {
            if (tacc_file_iter_accept_ch(iter, '=')) {
                ret->pp_tok__kind = TOK_GT_2_EQ;
                tacc_pp_tok_assign_str(ret, ">>=");
                return ret;
            } else {
                ret->pp_tok__kind = TOK_GT_2;
                tacc_pp_tok_assign_str(ret, ">>");
                return ret;
            }
        }
        break;
    case '=':
        ret->pp_tok__kind = TOK_EQ;
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
        tacc_assert(0, "unrecognized char: %x", first);
        kind = TOK_OTHER;
    }

    /* don't include possible following splice */
    ret->pp_tok_str = tacc_malloc(2);
    ret->pp_tok_end = ret->pp_tok_str + 1;
    ret->pp_tok_str[0] = first;
    ret->pp_tok_str[1] = 0;
    ret->pp_tok__kind = kind;
    return ret;
}

static pp_tok_p tacc_file_iter_expect_ident(tacc_file_iter_p iter) {
    pp_tok_p tok;
    char ch;

    tok = tacc_malloc(sizeof(struct pp_tok));
    tok->pp_tok_is_final = 0;
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
    int i;
    tacc_macro_def_entry_p entry;

    entry = state->tacc_pp_macros;
    for (i = 0; i < 1024; i = i + 1) {
        if (!entry->tacc_macro_def_entry_content) {
            /* first empty */
            return entry;
        }
        if (!strcmp(entry->tacc_macro_def_entry_content->tacc_macro_def_name,
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

    place =
        tacc_pp_find_macro_or_first_empty(state, macro->tacc_macro_def_name);
    if (place->tacc_macro_def_entry_content) {
        tacc_assert(
            place->tacc_macro_def_entry_content->tacc_macro_def_is_tombstone,
            "macro defined twice: %s",
            macro->tacc_macro_def_name);
    }
    place->tacc_macro_def_entry_content = macro;
}

void tacc_pp_define(tacc_pp_state_p state, char *name, char *expansion) {
    tacc_token_pp replacement_list;
    tacc_macro_def_p macro;
    tacc_file_iter_p iter;
    pp_tok_p tok;
    size_t replacement_list_len;

    macro = tacc_malloc(sizeof(struct tacc_macro_def));
    macro->tacc_macro_def_name = name;
    macro->tacc_macro_def_is_va = 0;
    macro->tacc_macro_def_is_tombstone = 0;
    macro->tacc_macro_def_num_params = 0;
    macro->tacc_macro_def_is_function_like = 0;
    macro->tacc_macro_def_params = NULL;

    replacement_list = tacc_malloc(sizeof(struct tacc_token_p) * 512);
    macro->tacc_macro_def_replacement_list = replacement_list;
    replacement_list_len = 0;

    iter = tacc_file_iter_new();
    tacc_file_iter_init_str(iter, expansion, expansion + strlen(expansion));

    while (1) {
        tacc_assert(replacement_list_len < 512, "overlong replacement list");
        tok = tacc_file_iter_lex(
            iter, 0 /* no directives, but produce TOK_SHARP{,_2} */, 0);
        if (tok->pp_tok__kind == TOK_EOF) {
            break;
        }
        replacement_list->tacc_token_p_content = tok;
        replacement_list_len = replacement_list_len + 1;
        replacement_list =
            replacement_list + tacc_sizeadj(1, sizeof(struct tacc_token_p));
    }
    macro->tacc_macro_def_replacement_list_len = replacement_list_len;

    tacc_pp_insert_macro(state, macro);
}

void tacc_pp_undef(tacc_pp_state_p state, char *name) {
    tacc_macro_def_entry_p place;

    place = tacc_pp_find_macro_or_first_empty(state, name);
    if (place->tacc_macro_def_entry_content == NULL) {
        return;
    }
    place->tacc_macro_def_entry_content->tacc_macro_def_is_tombstone = 1;
}

void tacc_pp_state_init(tacc_pp_state_p state) {
    int i;
    tacc_include_path_p incpath_entry;
    tacc_macro_def_entry_p macro_entry;

    state->tacc_pp_include_path =
        tacc_malloc(sizeof(struct tacc_include_path_entry) * 32);
    state->tacc_pp_macros =
        tacc_malloc(sizeof(struct tacc_macro_def_entry) * 1024);

    incpath_entry = state->tacc_pp_include_path;
    for (i = 0; i < 32; i = i + 1) {
        incpath_entry->tacc_include_path_entry_content = NULL;
        incpath_entry = incpath_entry +
                        tacc_sizeadj(1, sizeof(struct tacc_include_path_entry));
    }
    state->tacc_pp_include_path->tacc_include_path_entry_content = ".";
    macro_entry = state->tacc_pp_macros;
    for (i = 0; i < 1024; i = i + 1) {
        macro_entry->tacc_macro_def_entry_content = NULL;
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
    iter->tacc_tok_iter_file = file;
    iter->tacc_tok_iter_pending_len = 0;
    iter->tacc_tok_iter_pending =
        tacc_malloc(512 * sizeof(struct tacc_token_p));
    iter->tacc_tok_iter_override = NULL;
    iter->tacc_tok_iter_state = state;
    iter->tacc_tok_iter_inc_level = 0;
    iter->tacc_tok_iter_skip_level = 0;
    iter->tacc_tok_iter_in_macro_args = 0;
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
    printf("trying %s\n", path);

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
    int i;

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
    entry = state->tacc_pp_include_path;
    for (i = 0; i < 32; ++i) {
        if (entry->tacc_include_path_entry_content == NULL) {
            break;
        }
        try_file =
            tacc_pp_try_file(entry->tacc_include_path_entry_content, subpath);
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

    while (last_iter->tacc_tok_iter_override) {
        last_iter = last_iter->tacc_tok_iter_override;
    }
    return last_iter;
}

static void tacc_tok_iter_handle_include(tacc_tok_iter_p first,
                                         tacc_file_iter_p iter) {
    int is_angle;
    char ch;
    char *hdr_name;
    char *hdr_name_start;
    char *hdr_name_end;
    tacc_tok_iter_p last_iter;
    tacc_file_p included_file;
    tacc_file_iter_p included_file_iter;
    tacc_tok_iter_p included_file_tok_iter;

    if (tacc_file_iter_accept_ch(iter, '<')) {
        is_angle = 1;
    } else {
        is_angle = 0;
        tacc_assert(tacc_file_iter_accept_ch(iter, '"'), "expected < or \"");
    }

    hdr_name = tacc_malloc(1024);
    hdr_name_start = hdr_name;
    hdr_name_end = hdr_name + 1023;
    *hdr_name_end = 0;

    while (1) {
        tacc_assert(hdr_name != hdr_name_end, "overlong included file name");
        if (is_angle) {
            if (tacc_file_iter_accept_ch(iter, '>')) {
                hdr_name_end = hdr_name;
                break;
            }
        } else if (tacc_file_iter_accept_ch(iter, '"')) {
            hdr_name_end = hdr_name;
            break;
        }
        ch = tacc_file_iter_consume_ch(iter);
        *hdr_name = ch;
        hdr_name = hdr_name + 1;
    }
    *hdr_name = 0;

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #include");

    last_iter = tacc_tok_iter_cur_iter(first);

    included_file = tacc_pp_search_include_path(
        first->tacc_tok_iter_state,
        last_iter->tacc_tok_iter_file->tacc_file_iter_filename,
        hdr_name_start);
    included_file_iter = tacc_file_iter_new();
    tacc_file_iter_init(included_file_iter, included_file);
    included_file_tok_iter = tacc_tok_iter_new();
    tacc_tok_iter_init(
        included_file_tok_iter, included_file_iter, first->tacc_tok_iter_state);
    last_iter->tacc_tok_iter_override = included_file_tok_iter;
}

static void tacc_tok_iter_handle_define(tacc_tok_iter_p first,
                                        tacc_file_iter_p iter) {
    pp_tok_p tok;
    char *macro_name;
    tacc_macro_def_p macro;
    tacc_ident_p params;
    tacc_token_pp replacement_list;
    size_t i;
    size_t replacement_list_len;
    int terminated;

    macro = tacc_malloc(sizeof(struct tacc_macro_def));
    tok = tacc_file_iter_expect_ident(iter);
    macro_name = tok->pp_tok_str;
    macro->tacc_macro_def_name = macro_name;
    macro->tacc_macro_def_is_va = 0;
    macro->tacc_macro_def_is_tombstone = 0;
    macro->tacc_macro_def_is_function_like = 0;

    /* don't eat whitespace before lparen */
    if (tacc_file_iter_accept_ch(iter, '(')) {
        macro->tacc_macro_def_is_function_like = 1;
        tacc_file_iter_eat_ws_no_newlines(iter);
        /* function-like macro */
        params = tacc_malloc(8 * sizeof(struct tacc_ident));
        macro->tacc_macro_def_params = params;
        if (tacc_file_iter_accept_ch(iter, ')')) {
            macro->tacc_macro_def_num_params = 0;
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
                    macro->tacc_macro_def_num_params = i;
                    macro->tacc_macro_def_is_va = 1;
                    break;
                }
                tok = tacc_file_iter_expect_ident(iter);
                params->tacc_ident_content = tok->pp_tok_str;
                tacc_file_iter_eat_ws_no_newlines(iter);
                if (tacc_file_iter_accept_ch(iter, ',')) {
                    params = params + tacc_sizeadj(1, sizeof(tacc_ident_p));
                    continue;
                }
                tacc_assert(tacc_file_iter_accept_ch(iter, ')'),
                            "expected , or )");
                terminated = 1;
                macro->tacc_macro_def_num_params = i + 1;
                break;
            }
            tacc_assert(terminated, "overlong macro parameter list");
        }
        tacc_file_iter_eat_ws_no_newlines(iter);
    } else {
        tacc_file_iter_eat_ws_no_newlines(iter);
        macro->tacc_macro_def_num_params = 0;
        macro->tacc_macro_def_params = NULL;
    }
    /* no ws necessarily required here!!! huh??? */

    replacement_list = tacc_malloc(sizeof(struct tacc_token_p) * 512);
    macro->tacc_macro_def_replacement_list = replacement_list;
    replacement_list_len = 0;
    while (1) {
        tacc_assert(replacement_list_len < 512, "overlong replacement list");
        tok = tacc_file_iter_lex(
            iter, 0 /* no directives, but produce TOK_SHARP{,_2} */, 0);
        if (tok->pp_tok__kind == TOK_EOF) {
            break;
        }
        replacement_list->tacc_token_p_content = tok;
        replacement_list_len = replacement_list_len + 1;
        replacement_list =
            replacement_list + tacc_sizeadj(1, sizeof(struct tacc_token_p));
    }
    macro->tacc_macro_def_replacement_list_len = replacement_list_len;

    tacc_pp_insert_macro(first->tacc_tok_iter_state, macro);
}

static void tacc_tok_iter_handle_undef(tacc_tok_iter_p first,
                                       tacc_file_iter_p iter) {
    pp_tok_p tok;

    tok = tacc_file_iter_expect_ident(iter);
    tacc_pp_undef(first->tacc_tok_iter_state, tok->pp_tok_str);
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

    macro_def = tacc_pp_find_macro_or_first_empty(first->tacc_tok_iter_state,
                                                  tok->pp_tok_str);
    if (macro_def->tacc_macro_def_entry_content) {
        last_iter->tacc_tok_iter_inc_level =
            last_iter->tacc_tok_iter_inc_level + 1;
        return;
    }
    last_iter->tacc_tok_iter_skip_level =
        last_iter->tacc_tok_iter_skip_level + 1;
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

    macro_def = tacc_pp_find_macro_or_first_empty(first->tacc_tok_iter_state,
                                                  tok->pp_tok_str);
    if (!macro_def->tacc_macro_def_entry_content) {
        last_iter->tacc_tok_iter_inc_level =
            last_iter->tacc_tok_iter_inc_level + 1;
        return;
    }
    last_iter->tacc_tok_iter_skip_level =
        last_iter->tacc_tok_iter_skip_level + 1;
}

static void tacc_tok_iter_handle_endif(tacc_tok_iter_p first,
                                       tacc_file_iter_p iter) {
    tacc_tok_iter_p last_iter;

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #endif");

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->tacc_tok_iter_skip_level > 0) {
        last_iter->tacc_tok_iter_skip_level =
            last_iter->tacc_tok_iter_skip_level - 1;
        return;
    }
    tacc_assert(last_iter->tacc_tok_iter_inc_level > 0, "stray #endif");
    last_iter->tacc_tok_iter_inc_level = last_iter->tacc_tok_iter_inc_level - 1;
}

static void tacc_tok_iter_handle_else(tacc_tok_iter_p first,
                                      tacc_file_iter_p iter) {
    tacc_tok_iter_p last_iter;

    tacc_file_iter_eat_ws_no_newlines(iter);
    tacc_assert(tacc_file_is_eof(iter), "junk after #else");

    last_iter = tacc_tok_iter_cur_iter(first);

    if (last_iter->tacc_tok_iter_skip_level == 1) {
        last_iter->tacc_tok_iter_skip_level = 0;
        last_iter->tacc_tok_iter_inc_level =
            last_iter->tacc_tok_iter_inc_level + 1;
        return;
    }
    if (last_iter->tacc_tok_iter_skip_level > 1) {
        return;
    }
    tacc_assert(last_iter->tacc_tok_iter_inc_level > 0, "stray #else");
    last_iter->tacc_tok_iter_inc_level = last_iter->tacc_tok_iter_inc_level - 1;
    last_iter->tacc_tok_iter_skip_level =
        last_iter->tacc_tok_iter_skip_level + 1;
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

    tacc_assert(0, "#error: %s", iter->tacc_file_iter_src);

#ifdef __STDC__
    (void) first;
#endif
}

static void tacc_tok_iter_handle_directive(tacc_tok_iter_p first,
                                           char *directive,
                                           char *directive_end) {
    tacc_file_iter_p dir_scanner;
    pp_tok_p tok;

    dir_scanner = tacc_file_iter_new();
    tacc_file_iter_init_str(dir_scanner, directive, directive_end);

    tacc_file_iter_eat_ws_no_newlines(dir_scanner);
    if (tacc_file_is_eof(dir_scanner) ||
        tacc_file_iter_accept_ch(dir_scanner, '\n')) {
        /* empty directive, skip */
        return;
    }
    tok = tacc_file_iter_expect_ident(dir_scanner);
    tacc_file_iter_eat_ws_no_newlines(dir_scanner);

    if (!strcmp(tok->pp_tok_str, "include")) {
        tacc_assert(dir_scanner->tacc_file_iter_is_ws,
                    "expected whitespace after #include");
        tacc_tok_iter_handle_include(first, dir_scanner);
        return;
    }
    if (!strcmp(tok->pp_tok_str, "define")) {
        tacc_assert(dir_scanner->tacc_file_iter_is_ws,
                    "expected whitespace after #define");
        tacc_tok_iter_handle_define(first, dir_scanner);
        return;
    }
    if (!strcmp(tok->pp_tok_str, "undef")) {
        tacc_assert(dir_scanner->tacc_file_iter_is_ws,
                    "expected whitespace after #undef");
        tacc_tok_iter_handle_undef(first, dir_scanner);
        return;
    }
    if (!strcmp(tok->pp_tok_str, "if")) {
        tacc_tok_iter_handle_if(first, dir_scanner);
        return;
    }
    if (!strcmp(tok->pp_tok_str, "ifdef")) {
        tacc_assert(dir_scanner->tacc_file_iter_is_ws,
                    "expected whitespace after #ifdef");
        tacc_tok_iter_handle_ifdef(first, dir_scanner);
        return;
    }
    if (!strcmp(tok->pp_tok_str, "ifndef")) {
        tacc_assert(dir_scanner->tacc_file_iter_is_ws,
                    "expected whitespace after #ifndef");
        tacc_tok_iter_handle_ifndef(first, dir_scanner);
        return;
    }
    if (!strcmp(tok->pp_tok_str, "endif")) {
        tacc_tok_iter_handle_endif(first, dir_scanner);
        return;
    }
    if (!strcmp(tok->pp_tok_str, "else")) {
        tacc_tok_iter_handle_else(first, dir_scanner);
        return;
    }
    if (!strcmp(tok->pp_tok_str, "error")) {
        tacc_tok_iter_handle_error_directive(first, dir_scanner);
        return;
    }
    tacc_assert(0, "unknown directive: %s", tok->pp_tok_str);
}

static pp_tok_p tacc_tok_iter_peek_nomacro(tacc_tok_iter_p iter) {
    pp_tok_p tok;
    tacc_macro_def_entry_p macro_entry;
    tacc_macro_def_p macro_def;
    tacc_token_pp entry;

    if (iter->tacc_tok_iter_pending_len == 0) {
        if (iter->tacc_tok_iter_in_macro_args) {
            tok = tacc_file_iter_lex(iter->tacc_tok_iter_file, 0, 0);
        } else if (iter->tacc_tok_iter_skip_level > 0) {
            tok = tacc_file_iter_lex(iter->tacc_tok_iter_file, 1, 1);
        } else {
            tok = tacc_file_iter_lex(iter->tacc_tok_iter_file, 1, 0);
        }
        iter->tacc_tok_iter_pending->tacc_token_p_content = tok;
        iter->tacc_tok_iter_pending_len = 1;
        return tok;
    }
    entry = iter->tacc_tok_iter_pending +
            tacc_sizeadj((iter->tacc_tok_iter_pending_len - 1),
                         sizeof(struct tacc_token_p));
    tok = entry->tacc_token_p_content;
    if (tok->pp_tok__kind == TOK_FAKE_END_OF_MACRO) {
        iter->tacc_tok_iter_pending_len = iter->tacc_tok_iter_pending_len - 1;
        macro_entry = tacc_pp_find_macro_or_first_empty(
            iter->tacc_tok_iter_state, tok->pp_tok_str);
        macro_def = macro_entry->tacc_macro_def_entry_content;
        tacc_assert(macro_def != NULL,
                    "failed to find macrodef for macro being expanded: %s",
                    tok->pp_tok_str);
        macro_def->tacc_macro_def_is_replacing = 0;
        /* simple recursion, this shouldn't go too deep */
        return tacc_tok_iter_peek_nomacro(iter);
    }
    if (tok->pp_tok__kind == TOK_FAKE_PMARK) {
        iter->tacc_tok_iter_pending_len = iter->tacc_tok_iter_pending_len - 1;
        /* simple recursion, this shouldn't go too deep */
        return tacc_tok_iter_peek_nomacro(iter);
    }
    return tok;
}

static pp_tok_p tacc_tok_iter_consume_nomacro(tacc_tok_iter_p iter) {
    pp_tok_p tok = tacc_tok_iter_peek_nomacro(iter);
    iter->tacc_tok_iter_pending_len = iter->tacc_tok_iter_pending_len - 1;
    return tok;
}

static void tacc_tok_iter_push_pending(tacc_tok_iter_p iter, pp_tok_p tok) {
    tacc_token_pp tok_entry;

    tok_entry = iter->tacc_tok_iter_pending +
                tacc_sizeadj(iter->tacc_tok_iter_pending_len,
                             sizeof(struct tacc_token_p));
    tok_entry->tacc_token_p_content = tok;
    iter->tacc_tok_iter_pending_len = iter->tacc_tok_iter_pending_len + 1;
}

static void tacc_tok_iter_join_pending(tacc_tok_iter_p iter, pp_tok_p tok) {
    tacc_token_pp tok_entry;
    tacc_file_iter_p file_iter;
    pp_tok_p old_tok;
    pp_tok_p new_tok;
    char *new_tok_str;

    tok_entry = iter->tacc_tok_iter_pending +
                tacc_sizeadj(iter->tacc_tok_iter_pending_len - 1,
                             sizeof(struct tacc_token_p));
    old_tok = tok_entry->tacc_token_p_content;
    new_tok_str = tacc_malloc(1024);
    *new_tok_str = 0;

    /*
     * pending is pushed in right-to-left order.
     * Later tokens will already be present in the stack, so we join in reverse
     * order.
     */
    strcat(new_tok_str, tok->pp_tok_str);
    strcat(new_tok_str, old_tok->pp_tok_str);

    file_iter = tacc_file_iter_new();
    tacc_file_iter_init_str(file_iter,
                            new_tok_str,
                            new_tok_str +
                                (old_tok->pp_tok_end - old_tok->pp_tok_str) +
                                (tok->pp_tok_end - tok->pp_tok_str));

    new_tok = tacc_file_iter_lex(file_iter, 0, 0);
    new_tok->pp_tok_preceded_by_ws = tok->pp_tok_preceded_by_ws;
    tacc_file_iter_eat_ws_no_newlines(file_iter);
    tacc_assert(tacc_file_is_eof(file_iter),
                "multiple tokens produced by joining: %s",
                new_tok_str);

    tok_entry->tacc_token_p_content = new_tok;
}

static void tacc_tok_iter_insert_macro_replacing_stop(tacc_tok_iter_p iter,
                                                      char *macro_name) {
    pp_tok_p tok;

    tok = tacc_malloc(sizeof(struct pp_tok));
    tok->pp_tok__kind = TOK_FAKE_END_OF_MACRO;
    tok->pp_tok_str = macro_name;
    tok->pp_tok_end = macro_name + strlen(macro_name);
    tacc_tok_iter_push_pending(iter, tok);
}

static void tacc_tok_iter_push_placemarker(tacc_tok_iter_p iter) {
    pp_tok_p tok;

    tok = tacc_malloc(sizeof(struct pp_tok));
    tok->pp_tok__kind = TOK_FAKE_PMARK;
    tok->pp_tok_str = "";
    tok->pp_tok_end = tok->pp_tok_str;
    tacc_tok_iter_push_pending(iter, tok);
}

static size_t tacc_macro_def_index_of_par(tacc_macro_def_p macro_def,
                                          char *ident) {
    size_t i;
    tacc_ident_p par_ident;

    if (macro_def->tacc_macro_def_is_va && strcmp(ident, "__VA_ARGS__")) {
        return macro_def->tacc_macro_def_num_params;
    }
    for (i = 0; i < macro_def->tacc_macro_def_num_params; ++i) {
        par_ident = macro_def->tacc_macro_def_params +
                    tacc_sizeadj(i, sizeof(struct tacc_ident));
        if (!strcmp(par_ident->tacc_ident_content, ident)) {
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
    int nest_level;
    size_t i;
    size_t start_j;
    int had_comma;
    size_t j;
    size_t max_param;

    nest_level = 0;
    tacc_assert(macro_def->tacc_macro_def_is_function_like,
                "object-like macro takes no params");
    /* +1 to leave space for overflow, even if varargs are not used */
    ret = tacc_malloc((macro_def->tacc_macro_def_num_params + 1) *
                      sizeof(struct tacc_token_p_list));
    j = 0;
    max_param = macro_def->tacc_macro_def_num_params;
    if (macro_def->tacc_macro_def_is_va) {
        max_param = max_param + 1;
    }
    for (i = 0; i < max_param; i = i + 1) {
        ret_cur = ret + tacc_sizeadj(i, sizeof(struct tacc_token_p_list));
        if (j >= raw_args_len) {
            ret_cur->tacc_token_p_list_content = NULL;
            ret_cur->tacc_token_p_list_len = 0;
            continue;
        }
        start_j = j;
        had_comma = 0;
        while (j < raw_args_len) {
            raw_arg_entry =
                raw_args + tacc_sizeadj(j, sizeof(struct tacc_token_p));
            raw_arg = raw_arg_entry->tacc_token_p_content;

            if ((nest_level == 0) && (raw_arg->pp_tok__kind == TOK_COMMA) &&
                (i < macro_def->tacc_macro_def_num_params)) {
                /* if i == tacc_macro_def_num_params, we are looking at va_args
                 */
                had_comma = 1;
                j = j + 1;
                break;
            }
            if (raw_arg->pp_tok__kind == TOK_LPAREN) {
                nest_level = nest_level + 1;
            }
            if (raw_arg->pp_tok__kind == TOK_RPAREN) {
                nest_level = nest_level - 1;
            }
            j = j + 1;
        }
        tacc_assert(nest_level == 0, "missing ) in macro argument list");

        ret_cur->tacc_token_p_list_len = j - start_j;
        if (had_comma) {
            ret_cur->tacc_token_p_list_len = ret_cur->tacc_token_p_list_len - 1;
        }
        ret_cur->tacc_token_p_list_content = tacc_malloc(
            ret_cur->tacc_token_p_list_len * sizeof(struct tacc_token_p));
        memcpy(ret_cur->tacc_token_p_list_content,
               raw_args + tacc_sizeadj(start_j, sizeof(struct tacc_token_p)),
               ret_cur->tacc_token_p_list_len * sizeof(struct tacc_token_p));
    }

    return ret;
}

static pp_tok_p tacc_pp_stringify(tacc_token_pp tokens, size_t num_tokens) {
    pp_tok_p ret;
    char *ret_tok_str;
    char *this_tok_str;
    tacc_token_pp tok_entry;
    pp_tok_p tok;
    size_t i;
    size_t j;

    ret_tok_str = tacc_malloc(1024);
    ret = tacc_malloc(sizeof(pp_tok_p));
    ret->pp_tok__kind = TOK_STRING;
    ret->pp_tok_is_final = 0;
    ret->pp_tok_str = ret_tok_str;

    tok_entry = tokens;
    *ret_tok_str = '"';
    ret_tok_str = ret_tok_str + 1;
    *ret_tok_str = 0;
    for (i = 0; i < num_tokens; i = i + 1) {
        tok = tok_entry->tacc_token_p_content;
        if ((i != 0) && tok->pp_tok_preceded_by_ws) {
            *ret_tok_str = ' ';
            ret_tok_str = ret_tok_str + 1;
        }
        if ((tok->pp_tok__kind == TOK_STRING) ||
            (tok->pp_tok__kind == TOK_CHAR)) {
            this_tok_str = tok->pp_tok_str;
            for (j = 0; j < (size_t) (tok->pp_tok_end - this_tok_str);
                 j = j + 1) {
                if (*this_tok_str == '"') {
                    *ret_tok_str = '\\';
                    ret_tok_str = ret_tok_str + 1;
                    *ret_tok_str = '"';
                    ret_tok_str = ret_tok_str + 1;
                } else if (*this_tok_str == '\\') {
                    *ret_tok_str = '\\';
                    ret_tok_str = ret_tok_str + 1;
                    *ret_tok_str = '\\';
                    ret_tok_str = ret_tok_str + 1;
                } else {
                    *ret_tok_str = *this_tok_str;
                    ret_tok_str = ret_tok_str + 1;
                }
                this_tok_str = this_tok_str + 1;
            }
        } else {
            this_tok_str = tok->pp_tok_str;
            strcat(ret_tok_str, this_tok_str);
            ret_tok_str = ret_tok_str + (tok->pp_tok_end - tok->pp_tok_str);
        }
        tok_entry = tok_entry + tacc_sizeadj(1, sizeof(struct tacc_token_p));
        *ret_tok_str = 0;
    }
    *ret_tok_str = '"';
    ret_tok_str = ret_tok_str + 1;
    *ret_tok_str = 0;
    ret->pp_tok_end = ret_tok_str;

    return ret;
}

static void tacc_tok_iter_push_all_expanding(tacc_tok_iter_p iter,
                                             tacc_token_pp tokens,
                                             size_t num_tokens,
                                             int first_preceded_by_ws,
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
    helper_iter->tacc_tok_iter_file = iter->tacc_tok_iter_file;
    helper_iter->tacc_tok_iter_inc_level = 0;
    helper_iter->tacc_tok_iter_skip_level = 0;
    helper_iter->tacc_tok_iter_override = NULL;
    helper_iter->tacc_tok_iter_state = iter->tacc_tok_iter_state;
    helper_iter->tacc_tok_iter_in_macro_args = 0;

    helper_iter->tacc_tok_iter_pending =
        tacc_malloc(512 * sizeof(struct tacc_token_p));
    helper_iter->tacc_tok_iter_pending_len = 0;

    helper_eof = tacc_malloc(sizeof(struct pp_tok));
    helper_eof->pp_tok__kind = TOK_EOF;
    helper_eof->pp_tok_is_final = 1;
    helper_eof->pp_tok_preceded_by_ws = 1;
    tacc_tok_iter_push_pending(helper_iter, helper_eof);

    /* Soak the sponge */
    token_cur = tokens + tacc_sizeadj(num_tokens, sizeof(struct tacc_token_p));
    for (i = 0; i < num_tokens; i = i + 1) {
        token_cur = token_cur - tacc_sizeadj(1, sizeof(struct tacc_token_p));
        tacc_tok_iter_push_pending(helper_iter,
                                   token_cur->tacc_token_p_content);
    }

    /* Squeeze the sponge */
    macro_def->tacc_macro_def_is_replacing = 0;
    reverse_toks = tacc_malloc(512 * sizeof(struct tacc_token_p));
    tok_count = 0;
    while (1) {
        tok = tacc_tok_iter_next(helper_iter);
        if (tok->pp_tok__kind == TOK_EOF) {
            break;
        }
        if ((tok_count == 0) &&
            (tok->pp_tok_preceded_by_ws != first_preceded_by_ws)) {
            new_tok = tacc_malloc(sizeof(struct pp_tok));
            memcpy(new_tok, tok, sizeof(struct pp_tok));
            new_tok->pp_tok_preceded_by_ws = first_preceded_by_ws;
            tok = new_tok;
        }
        reverse_toks->tacc_token_p_content = tok;
        reverse_toks =
            reverse_toks + tacc_sizeadj(1, sizeof(struct tacc_token_p));
        tok_count = tok_count + 1;
    }
    /* Reverse order, for buffer */
    for (i = 0; i < tok_count; i = i + 1) {
        reverse_toks =
            reverse_toks - tacc_sizeadj(1, sizeof(struct tacc_token_p));
        tacc_tok_iter_push_pending(iter, reverse_toks->tacc_token_p_content);
    }
    macro_def->tacc_macro_def_is_replacing = 1;
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
        macro_def->tacc_macro_def_replacement_list +
        tacc_sizeadj(macro_def->tacc_macro_def_replacement_list_len,
                     sizeof(struct tacc_token_p));
    for (i = 0; i < macro_def->tacc_macro_def_replacement_list_len; i = i + 1) {
        replacing_tok_entry =
            replacing_tok_entry - tacc_sizeadj(1, sizeof(struct tacc_token_p));
        replacing_tok = replacing_tok_entry->tacc_token_p_content;
        tacc_assert(replacing_tok->pp_tok__kind != TOK_SHARP,
                    "stray # in function-like macro expansion list");
        if (replacing_tok->pp_tok__kind == TOK_SHARP_2) {
            i = i + 1;
            replacing_tok_entry = replacing_tok_entry -
                                  tacc_sizeadj(1, sizeof(struct tacc_token_p));
            tacc_assert(i < macro_def->tacc_macro_def_replacement_list_len,
                        "## on edge of function-like macro expansion list");
            next_tok = replacing_tok_entry->tacc_token_p_content;
            if (next_tok->pp_tok__kind != TOK_IDENT) {
                tacc_tok_iter_join_pending(iter_within, next_tok);
                continue;
            }
            par_position =
                tacc_macro_def_index_of_par(macro_def, next_tok->pp_tok_str);
            if (par_position == TACC_PARAM_NOT_FOUND) {
                tacc_tok_iter_join_pending(iter_within, next_tok);
                continue;
            }
            arg_entry =
                split_args +
                tacc_sizeadj(par_position, sizeof(struct tacc_token_p_list));
            arg = arg_entry->tacc_token_p_list_content;
            if (arg_entry->tacc_token_p_list_len == 0) {
                tacc_tok_iter_push_placemarker(iter_within);
                continue;
            }
            arg_last_tok =
                arg + tacc_sizeadj(arg_entry->tacc_token_p_list_len - 1,
                                   sizeof(struct tacc_token_p));
            tacc_tok_iter_join_pending(iter_within,
                                       arg_last_tok->tacc_token_p_content);
            if (arg_entry->tacc_token_p_list_len > 1) {
                tacc_tok_iter_push_all_expanding(
                    iter_within,
                    arg,
                    arg_entry->tacc_token_p_list_len - 1,
                    next_tok->pp_tok_preceded_by_ws,
                    macro_def);
            }
            continue;
        }
        if (replacing_tok->pp_tok__kind != TOK_IDENT) {
            tacc_tok_iter_push_pending(iter_within, replacing_tok);
            continue;
        }
        par_position =
            tacc_macro_def_index_of_par(macro_def, replacing_tok->pp_tok_str);
        if (par_position == TACC_PARAM_NOT_FOUND) {
            tacc_tok_iter_push_pending(iter_within, replacing_tok);
            continue;
        }
        arg_entry = split_args + tacc_sizeadj(par_position,
                                              sizeof(struct tacc_token_p_list));
        arg = arg_entry->tacc_token_p_list_content;
        if (i == macro_def->tacc_macro_def_replacement_list_len - 1) {
            if (arg_entry->tacc_token_p_list_len == 0) {
                /* nothing to do */
                continue;
            }
            tacc_tok_iter_push_all_expanding(iter_within,
                                             arg,
                                             arg_entry->tacc_token_p_list_len,
                                             0,
                                             macro_def);
            continue;
        }
        next_tok_entry =
            replacing_tok_entry - tacc_sizeadj(1, sizeof(struct tacc_token_p));
        next_tok = next_tok_entry->tacc_token_p_content;
        if (next_tok->pp_tok__kind == TOK_SHARP_2) {
            if (arg_entry->tacc_token_p_list_len == 0) {
                tacc_tok_iter_push_placemarker(iter_within);
                continue;
            }
            tacc_tok_iter_push_all_expanding(
                iter_within,
                arg + tacc_sizeadj(1, sizeof(struct tacc_token_p)),
                arg_entry->tacc_token_p_list_len - 1,
                replacing_tok->pp_tok_preceded_by_ws,
                macro_def);
            tacc_tok_iter_push_pending(iter_within, arg->tacc_token_p_content);
            continue;
        }
        if (next_tok->pp_tok__kind == TOK_SHARP) {
            tacc_tok_iter_push_pending(
                iter_within,
                tacc_pp_stringify(arg, arg_entry->tacc_token_p_list_len));

            /* consume */
            i = i + 1;
            replacing_tok_entry = replacing_tok_entry -
                                  tacc_sizeadj(1, sizeof(struct tacc_token_p));

            continue;
        }
        if (arg_entry->tacc_token_p_list_len == 0) {
            /* nothing to do */
            continue;
        }
        tacc_tok_iter_push_all_expanding(iter_within,
                                         arg,
                                         arg_entry->tacc_token_p_list_len,
                                         replacing_tok->pp_tok_preceded_by_ws,
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
    int nest_level;

    while (1) {
        tok = tacc_tok_iter_peek_nomacro(iter);
        if (tok->pp_tok__kind != TOK_IDENT) {
            return tok;
        }
        if (tok->pp_tok_is_final) {
            return tok;
        }

        macro_entry = tacc_pp_find_macro_or_first_empty(
            iter->tacc_tok_iter_state, tok->pp_tok_str);
        macro_def = macro_entry->tacc_macro_def_entry_content;
        if (!macro_def) {
            return tok;
        }
        if (macro_def->tacc_macro_def_is_tombstone) {
            return tok;
        }
        if (macro_def->tacc_macro_def_is_replacing) {
            new_tok = tacc_malloc(sizeof(struct pp_tok));
            memcpy(new_tok, tok, sizeof(struct pp_tok));
            new_tok->pp_tok_is_final = 1;
            return new_tok;
        }

        iter->tacc_tok_iter_pending_len = iter->tacc_tok_iter_pending_len - 1;

        if (!macro_def->tacc_macro_def_is_function_like) {
            macro_def->tacc_macro_def_is_replacing = 1;

            tacc_tok_iter_insert_macro_replacing_stop(
                iter, macro_def->tacc_macro_def_name);
            src_tok_entry =
                macro_def->tacc_macro_def_replacement_list +
                tacc_sizeadj(macro_def->tacc_macro_def_replacement_list_len - 1,
                             sizeof(struct tacc_token_p));
            for (i = 0; i < macro_def->tacc_macro_def_replacement_list_len;
                 i = i + 1) {
                if (src_tok_entry->tacc_token_p_content->pp_tok__kind ==
                    TOK_SHARP_2) {
                    tacc_assert(i != 0,
                                "unexpected ## at beginning of replacement "
                                "list of object-like macro");
                    tacc_assert(
                        i != macro_def->tacc_macro_def_replacement_list_len - 1,
                        "unexpected ## at end of replacement list of "
                        "object-like macro");

                    i = i + 1;
                    src_tok_entry =
                        src_tok_entry -
                        tacc_sizeadj(1, sizeof(struct tacc_token_p));
                    tacc_tok_iter_join_pending(
                        iter, src_tok_entry->tacc_token_p_content);
                } else {
                    tacc_tok_iter_push_pending(
                        iter, src_tok_entry->tacc_token_p_content);
                }

                src_tok_entry = src_tok_entry -
                                tacc_sizeadj(1, sizeof(struct tacc_token_p));
            }
            continue;
        }
        new_tok = tacc_tok_iter_peek_nomacro(iter);
        if (new_tok->pp_tok__kind != TOK_LPAREN) {
            /*
             * Not matched as a function-like macro; push `tok` back into queue.
             * To prevent an infinite loop, immediately return `tok`.
             */
            tacc_tok_iter_push_pending(iter, tok);
            return tok;
        }

        /* consume new_tok (lparen) */
        iter->tacc_tok_iter_pending_len = iter->tacc_tok_iter_pending_len - 1;

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
            iter->tacc_tok_iter_in_macro_args = 1;
            tok = tacc_tok_iter_consume_nomacro(iter);
            iter->tacc_tok_iter_in_macro_args = 0;
            tacc_assert(tok->pp_tok__kind != TOK_EOF,
                        "unmatched paren while invoking function-like macro");
            if (tok->pp_tok__kind == TOK_LPAREN) {
                nest_level = nest_level + 1;
            }
            if (tok->pp_tok__kind == TOK_RPAREN) {
                nest_level = nest_level - 1;
            }
            if (nest_level != 0) {
                arg_list_cur->tacc_token_p_content = tok;
                i = i + 1;
            }
            arg_list_cur =
                arg_list_cur + tacc_sizeadj(1, sizeof(struct tacc_token_p));
        }

        tacc_tok_iter_insert_macro_replacing_stop(
            iter, macro_def->tacc_macro_def_name);
        macro_def->tacc_macro_def_is_replacing = 1;

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
        while (last_iter->tacc_tok_iter_override) {
            prev_iter = last_iter;
            last_iter = last_iter->tacc_tok_iter_override;
        }

        /* ensure there is a token saved in peek buffer */
        peek_tok = tacc_tok_iter_peek_handle_macros(last_iter);
        if (peek_tok->pp_tok__kind == TOK_EOF) {
            tacc_assert(last_iter->tacc_tok_iter_inc_level == 0,
                        "missing #endif, including at level %d",
                        last_iter->tacc_tok_iter_inc_level);
            tacc_assert(last_iter->tacc_tok_iter_skip_level == 0,
                        "missing #endif, skipping at level %d",
                        last_iter->tacc_tok_iter_skip_level);
            /* EOF of current file; consume */
            if (!prev_iter) {
                /* base case: completely EOF, return EOF */
                return peek_tok;
            }

            /* go up a level */
            last_iter = prev_iter;
            last_iter->tacc_tok_iter_override = NULL;

            /* return to scanning from first iterator */
            continue;
        }
        if (peek_tok->pp_tok__kind != TOK_DIRECTIVE) {
            /* not a directive, pass through */
            return peek_tok;
        }

        /* going to handle a directive, so it's no longer pending */
        tacc_tok_iter_consume_nomacro(last_iter);

        /* Handle directive. This will never skip over any tokens. */
        tacc_tok_iter_handle_directive(
            first, peek_tok->pp_tok_str, peek_tok->pp_tok_end);
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

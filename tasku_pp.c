#include <string.h>
#include "util.h"
#include "tasku_pp.h"

tacc_file_iter_p tacc_file_iter_new(void) {
    return tacc_malloc(sizeof(struct tacc_file_iter));
}

void tacc_file_iter_init(tacc_file_iter_p iter, tacc_file_p file) {
    iter->tacc_file_iter_is_bol = 1;
    iter->tacc_file_iter_is_ws = 1;
    iter->tacc_file_iter_src = file->tacc_file_src;
    iter->tacc_file_iter_end = file->tacc_file_src + file->tacc_file_len;
}

char *tacc_pp_to_string(pp_tok_p tok) {
    switch (tok->pp_tok__kind) {
    case TOK_DIRECTIVE:
        return "TOK_DIRECTIVE";
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
    case TOK_LPAREN_NOWS:
        return "TOK_LPAREN_NOWS";
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
    }
    return "UNRECOGNIZED";
}

static void tacc_file_iter_eat_splices(tacc_file_iter_p iter) {
    char ch;
    ch = *iter->tacc_file_iter_src;

    if (ch == '\\') {
        /* c99 5.1.1.2.2: a physical source file cannot end with a backslash */
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

    tacc_assert(tacc_file_iter_accept_ch(iter, '/'), "error while scanning comment");
    tacc_assert(tacc_file_iter_accept_ch(iter, '*'), "error while scanning comment");

    while (1) {
        tacc_assert(!tacc_file_is_eof(iter), "eof in comment");
        ch = tacc_file_iter_consume_ch(iter);
        if (ch == '*') {
            if (tacc_file_iter_accept_ch(iter, '/')) {
                return;
            }
        }
    }
    iter->tacc_file_iter_is_ws = 1;
}

static void tacc_file_iter_eat_new_comment(tacc_file_iter_p iter) {
    char ch;

    tacc_assert(tacc_file_iter_accept_ch(iter, '/'), "error while scanning new comment");
    tacc_assert(tacc_file_iter_accept_ch(iter, '/'), "error while scanning new comment");

    ch = tacc_file_iter_peek_ch(iter);
    while (ch != '\n') {
        tacc_file_iter_consume_ch(iter);
        ch = tacc_file_iter_peek_ch(iter);
    }
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
            }
            /* do not eat double-slash comments; the newline is not part of them */
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
            /* do not eat double-slash comments; the newline is not part of them */
        }
        break;
    }
}

static char tacc_file_iter_parse_escape(tacc_file_iter_p iter) {
    char ch;
    char ret;

    ch = tacc_file_iter_consume_ch(iter);
    switch (ch) {
    case '\\':
    case '"':
    case '\'':
    case '?':
        return ch;
    case 'a':
        return (char) 7;
    case 'b':
        return (char) 8;
    case 'e':
        return (char) 27;
    case 'f':
        return (char) 12;
    case 'n':
        return (char) 10;
    case 'r':
        return (char) 14;
    case 't':
        return (char) 9;
    case 'v':
        return (char) 11;
    case 'x':
        ret = tacc_hex_to_dec(tacc_file_iter_consume_ch(iter));
        ret = ret * 16 + tacc_hex_to_dec(tacc_file_iter_consume_ch(iter));
        return ret;
    default:
        tacc_assert(ch >= '0' && ch <= '7', "invalid escape %x", ch);
        /* read 1 to 3 octal chars */
        ret = ch - '0';
        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '7') {
            ret = ret * 8 + (ch - '0');
        } else {
            return ret;
        }
        tacc_file_iter_consume_ch(iter);
        ch = tacc_file_iter_peek_ch(iter);
        if (ch >= '0' && ch <= '7') {
            tacc_file_iter_consume_ch(iter);
            ret = ret * 8 + (ch - '0');
        }
        return ret;
    }
}

static int tacc_tok_iter_lex_directive(tacc_file_iter_p iter, pp_tok_p tok_out) {
    char *directive_str;
    char *directive_str_end;
    int was_bol;

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
    while (!tacc_file_iter_accept_ch(iter, '\n')) {
        tacc_assert(directive_str != directive_str_end, "overlong directive");
        *directive_str = tacc_file_iter_consume_ch(iter);
        directive_str = directive_str + 1;
    }

    tok_out->pp_tok__kind = TOK_DIRECTIVE;
    *directive_str = 0;
    tok_out->pp_tok_end = directive_str;

    return 1;
}

static pp_tok_p tacc_tok_iter_lex_char(tacc_file_iter_p iter, pp_tok_p tok_out) {
    /* not permitted to match a partial or malformed character per 6.4p3 */
    pp_tok_p ret = tok_out;
    int contained;
    char *out_str;

    ret->pp_tok__kind = TOK_CHAR;
    out_str = tacc_malloc(2);
    out_str[1] = 0;
    ret->pp_tok_str = out_str;
    ret->pp_tok_end = out_str + 1;

    tacc_assert(!tacc_file_iter_accept_ch(iter, '\''), "empty character literal");
    if (!tacc_file_iter_accept_ch(iter, '\\')) {
        contained = tacc_file_iter_consume_ch(iter);
        tacc_assert(tacc_file_iter_accept_ch(iter, '\''), "overlong character literal");

        *out_str = (char) contained;
        return ret;
    }
    *out_str = tacc_file_iter_parse_escape(iter);
    tacc_assert(tacc_file_iter_accept_ch(iter, '\''), "overlong character literal");

    return ret;
}

static pp_tok_p tacc_tok_iter_lex_string(tacc_file_iter_p iter, pp_tok_p tok_out) {
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
        tacc_assert(!tacc_file_iter_accept_ch(iter, '\n'), "newline in string literal");
        tacc_assert(out_str != ret->pp_tok_end, "overlong string literal");
        if (!tacc_file_iter_accept_ch(iter, '\\')) {
            *out_str = tacc_file_iter_consume_ch(iter);
            out_str = out_str + 1;
            continue;
        }
        *out_str = tacc_file_iter_parse_escape(iter);
        out_str = out_str + 1;
    }
    *out_str = 0;
    ret->pp_tok_end = out_str;

    return ret;
}

static pp_tok_p tacc_tok_iter_lex_ppnum(tacc_file_iter_p iter, pp_tok_p tok_out, char first) {
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
        tacc_assert(ret->pp_tok_str != ret->pp_tok_end, "overlong ppnumber literal");
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

static pp_tok_p tacc_tok_iter_lex_ident(tacc_file_iter_p iter, pp_tok_p tok_out, char first) {
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
        tacc_assert(ret->pp_tok_str != ret->pp_tok_end, "overlong ident");
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

    return ret;
}

static void tacc_pp_tok_assign_str(pp_tok_p tok, char *str) {
    tok->pp_tok_str = str;
    tok->pp_tok_end = str + strlen(str);
}

static int tacc_tok_iter_maybe_special(tacc_tok_iter_p iter, pp_tok_p tok_out, pp_tok_kind_e special_tok, char *special_match) {
    if (!tacc_file_iter_accept_ch(iter->tacc_tok_iter_file, special_match[1])) {
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

static pp_tok_p tacc_tok_iter_lex(tacc_tok_iter_p iter) {
    int first, was_ws;
    pp_tok_p ret;
    pp_tok_kind_e kind;

    ret = tacc_malloc(sizeof(struct pp_tok));

    if (tacc_file_is_eof(iter->tacc_tok_iter_file)) {
        ret->pp_tok__kind = TOK_EOF;
        tacc_pp_tok_assign_str(ret, "");
        return ret;
    }

    if (tacc_tok_iter_lex_directive(iter->tacc_tok_iter_file, ret)) {
        return ret;
    }
    tacc_file_iter_eat_all_ws(iter->tacc_tok_iter_file);
    was_ws = iter->tacc_tok_iter_file->tacc_file_iter_is_ws;

    if (tacc_file_is_eof(iter->tacc_tok_iter_file)) {
        ret->pp_tok__kind = TOK_EOF;
        tacc_pp_tok_assign_str(ret, "");
        return ret;
    }

    kind = TOK_UNRECOGNIZED;

    first = tacc_file_iter_consume_ch(iter->tacc_tok_iter_file);
    if (!first) {
        return NULL;
    }

    switch (first) {
    case '\'':
        return tacc_tok_iter_lex_char(iter->tacc_tok_iter_file, ret);
    case '"':
        return tacc_tok_iter_lex_string(iter->tacc_tok_iter_file, ret);
    case '[':
        kind = TOK_LBRACE;
        break;
    case ']':
        kind = TOK_RBRACE;
        break;
    case '(':
        if (!was_ws) {
            kind = TOK_LPAREN_NOWS;
        } else {
            kind = TOK_LPAREN;
        }
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
        first = tacc_file_iter_peek_ch(iter->tacc_tok_iter_file);
        if (first >= '0' && first <= '9') {
            return tacc_tok_iter_lex_ppnum(iter->tacc_tok_iter_file, ret, '.');
        }
        if (first == '.' && ((iter->tacc_tok_iter_file->tacc_file_iter_end - iter->tacc_tok_iter_file->tacc_file_iter_src) >= 2) && iter->tacc_tok_iter_file->tacc_file_iter_src[1] == '.') {
            ret->pp_tok__kind = TOK_DOT_3;
            tacc_pp_tok_assign_str(ret, "...");
            return ret;
        }
        first = '.';
        break;
    case '-':
        if (tacc_tok_iter_maybe_special(iter, ret, TOK_ARROW, "->")) {
            return ret;
        }
        if (tacc_tok_iter_maybe_special(iter, ret, TOK_MINUS_2, "--")) {
            return ret;
        }
        ret->pp_tok__kind = TOK_MINUS;
        tacc_tok_iter_maybe_special(iter, ret, TOK_MINUS_EQ, "-=");
        return ret;
    case '&':
        if (tacc_tok_iter_maybe_special(iter, ret, TOK_AMPERSAND_2, "&&")) {
            return ret;
        }
        ret->pp_tok__kind = TOK_AMPERSAND;
        tacc_tok_iter_maybe_special(iter, ret, TOK_AMPERSAND_EQ, "&=");
        return ret;
    case '*':
        ret->pp_tok__kind = TOK_ASTERISK;
        tacc_tok_iter_maybe_special(iter, ret, TOK_ASTERISK_EQ, "*=");
        return ret;
    case '+':
        if (tacc_tok_iter_maybe_special(iter, ret, TOK_PLUS_2, "++")) {
            return ret;
        }
        ret->pp_tok__kind = TOK_PLUS;
        tacc_tok_iter_maybe_special(iter, ret, TOK_PLUS_EQ, "+=");
        return ret;
    case '~':
        kind = TOK_TILDE;
        break;
    case '!':
        ret->pp_tok__kind = TOK_EXCLAMATION;
        tacc_tok_iter_maybe_special(iter, ret, TOK_EXCLAMATION_EQ, "!=");
        return ret;
    case '^':
        ret->pp_tok__kind = TOK_CIRCUMFLEX;
        tacc_tok_iter_maybe_special(iter, ret, TOK_CIRCUMFLEX_EQ, "^=");
        return ret;
    case '|':
        if (tacc_tok_iter_maybe_special(iter, ret, TOK_PIPE_EQ, "|=")) {
            return ret;
        }
        ret->pp_tok__kind = TOK_PIPE;
        tacc_tok_iter_maybe_special(iter, ret, TOK_PIPE_2, "||");
        return ret;
    case '/':
        ret->pp_tok__kind = TOK_SLASH;
        tacc_tok_iter_maybe_special(iter, ret, TOK_SLASH_EQ, "/=");
        return ret;
    case '%':
        ret->pp_tok__kind = TOK_PERCENT;
        tacc_tok_iter_maybe_special(iter, ret, TOK_PERCENT_EQ, "%=");
        return ret;
    case '<':
        kind = TOK_LT;
        if (tacc_tok_iter_maybe_special(iter, ret, TOK_LT_EQ, "<=")) {
            return ret;
        }
        if (tacc_file_iter_accept_ch(iter->tacc_tok_iter_file, '<')) {
            if (tacc_file_iter_accept_ch(iter->tacc_tok_iter_file, '=')) {
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
        if (tacc_tok_iter_maybe_special(iter, ret, TOK_GT_EQ, "<=")) {
            return ret;
        }
        if (tacc_file_iter_accept_ch(iter->tacc_tok_iter_file, '>')) {
            if (tacc_file_iter_accept_ch(iter->tacc_tok_iter_file, '=')) {
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
        tacc_tok_iter_maybe_special(iter, ret, TOK_EQ_2, "==");
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
            return tacc_tok_iter_lex_ppnum(iter->tacc_tok_iter_file, ret, first);
        }
        if (first >= 'A' && first <= 'Z') {
            return tacc_tok_iter_lex_ident(iter->tacc_tok_iter_file, ret, first);
        }
        if (first >= 'a' && first <= 'z') {
            return tacc_tok_iter_lex_ident(iter->tacc_tok_iter_file, ret, first);
        }
        if (first == '_') {
            return tacc_tok_iter_lex_ident(iter->tacc_tok_iter_file, ret, first);
        }
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

tacc_tok_iter_p tacc_tok_iter_new(void) {
    return tacc_malloc(sizeof(struct tacc_tok_iter));
}

void tacc_tok_iter_init(tacc_tok_iter_p iter, tacc_file_iter_p file) {
    iter->tacc_tok_iter_file = file;
    iter->tacc_tok_iter_pending = NULL;
}

pp_tok_p tacc_tok_iter_peek(tacc_tok_iter_p iter) {
    if (iter->tacc_tok_iter_pending) {
        return iter->tacc_tok_iter_pending;
    }
    iter->tacc_tok_iter_pending = tacc_tok_iter_lex(iter);
    return iter->tacc_tok_iter_pending;
}

pp_tok_p tacc_tok_iter_next(tacc_tok_iter_p iter) {
    pp_tok_p ret;
    if (iter->tacc_tok_iter_pending) {
        ret = iter->tacc_tok_iter_pending;
        iter->tacc_tok_iter_pending = NULL;
        return ret;
    }
    return tacc_tok_iter_lex(iter);
}

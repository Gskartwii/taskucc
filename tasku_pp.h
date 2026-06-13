#ifndef TASKU_PP_H
#define TASKU_PP_H

#include "tasku_file.h"

enum pp_tok_kind {
    TOK_DIRECTIVE,
    TOK_CHAR,
    TOK_STRING,
    TOK_PPNUM,
    TOK_IDENT,
    TOK_EOF,
    TOK_UNRECOGNIZED,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN_NOWS,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_DOT,
    TOK_DOT_3,
    TOK_MINUS,
    TOK_ARROW,
    TOK_MINUS_2,
    TOK_MINUS_EQ,
    TOK_AMPERSAND,
    TOK_AMPERSAND_2,
    TOK_AMPERSAND_EQ,
    TOK_ASTERISK,
    TOK_ASTERISK_EQ,
    TOK_PLUS,
    TOK_PLUS_2,
    TOK_PLUS_EQ,
    TOK_TILDE,
    TOK_EXCLAMATION,
    TOK_EXCLAMATION_EQ,
    TOK_CIRCUMFLEX,
    TOK_CIRCUMFLEX_EQ,
    TOK_PIPE,
    TOK_PIPE_EQ,
    TOK_PIPE_2,
    TOK_SLASH,
    TOK_SLASH_EQ,
    TOK_PERCENT,
    TOK_PERCENT_EQ,
    TOK_LT,
    TOK_LT_EQ,
    TOK_LT_2_EQ,
    TOK_LT_2,
    TOK_GT,
    TOK_GT_EQ,
    TOK_GT_2_EQ,
    TOK_GT_2,
    TOK_EQ,
    TOK_EQ_2,
    TOK_QUESTION,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_OTHER
};

typedef enum pp_tok_kind pp_tok_kind_e;

struct tacc_file_iter {
    char *tacc_file_iter_src;
    char *tacc_file_iter_end;
    int tacc_file_iter_is_bol;
    int tacc_file_iter_is_ws;
};

typedef struct tacc_file_iter *tacc_file_iter_p; 
tacc_file_iter_p tacc_file_iter_new(void);
void tacc_file_iter_init(tacc_file_iter_p iter, tacc_file_p file);

struct pp_tok {
    pp_tok_kind_e pp_tok__kind;
    char *pp_tok_str;
    /* points at final null byte, does not include any possible intermediate null bytes */
    char *pp_tok_end;
};
typedef struct pp_tok *pp_tok_p;
char *tacc_pp_to_string(pp_tok_p tok);

struct tacc_tok_iter {
    tacc_file_iter_p tacc_tok_iter_file;
    pp_tok_p tacc_tok_iter_pending;
};
typedef struct tacc_tok_iter *tacc_tok_iter_p;

tacc_tok_iter_p tacc_tok_iter_new(void);
void tacc_tok_iter_init(tacc_tok_iter_p iter, tacc_file_iter_p file);
pp_tok_p tacc_tok_iter_peek(tacc_tok_iter_p iter);
pp_tok_p tacc_tok_iter_next(tacc_tok_iter_p iter);

#endif

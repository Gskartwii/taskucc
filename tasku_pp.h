#ifndef TASKU_PP_H
#define TASKU_PP_H

#include "tasku_file.h"

enum pp_tok_kind {
    TOK_DIRECTIVE,
    TOK_SHARP,
    TOK_SHARP_2,
    TOK_CHAR,
    TOK_STRING,
    TOK_PPNUM,
    TOK_IDENT,
    TOK_EOF,
    TOK_UNRECOGNIZED,
    TOK_LBRACE,
    TOK_RBRACE,
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

enum pp_ident_kind {
    ID_OTHER,

    ID_AUTO,
    ID_BREAK,
    ID_CASE,
    ID_CHAR,
    ID_CONST,
    ID_CONTINUE,
    ID_DEFAULT,
    ID_DO,
    ID_DOUBLE,
    ID_ENUM,
    ID_EXTERN,
    ID_FLOAT,
    ID_FOR,
    ID_GOTO,
    ID_INLINE,
    ID_INT,
    ID_LONG,
    ID_REGISTER,
    ID_RESTRICT,
    ID_RETURN,
    ID_SHORT,
    ID_SIGNED,
    ID_SIZEOF,
    ID_STATIC,
    ID_STRUCT,
    ID_SWITCH,
    ID_TYPEDEF,
    ID_UNION,
    ID_UNSIGNED,
    ID_VOID,
    ID_VOLATILE,
    ID_WHILE,
    ID__BOOL,
    ID__COMPLEX,
    ID__IMAGINARY,

    ID_ALWAYS_SPECIAL,

    ID_IF,
    ID_ELSE,

    ID_DIRECTIVE_SPECIALS,

    ID_IFDEF,
    ID_IFNDEF,
    ID_ELIF,
    ID_ENDIF,
    ID_INCLUDE,
    ID_DEFINE,
    ID_UNDEF,
    ID_LINE,
    ID_ERROR,
    ID_PRAGMA
};

typedef enum pp_ident_kind pp_ident_kind_e;

struct tacc_file_iter {
    char *tacc_file_iter_src;
    char *tacc_file_iter_end;
    char *tacc_file_iter_filename;
    int tacc_file_iter_is_bol;
    int tacc_file_iter_is_ws;
};

typedef struct tacc_file_iter *tacc_file_iter_p; 
tacc_file_iter_p tacc_file_iter_new(void);
void tacc_file_iter_init(tacc_file_iter_p iter, tacc_file_p file);

struct pp_tok {
    pp_tok_kind_e pp_tok__kind;
    pp_ident_kind_e pp_tok_ident_kind;
    char *pp_tok_str;
    /* points at final null byte, past any possible intermediate null bytes */
    char *pp_tok_end;

    int pp_tok_preceded_by_ws;
};
typedef struct pp_tok *pp_tok_p;
char *tacc_pp_to_string(pp_tok_p tok);

struct tacc_ident {
    char *tacc_ident_content;
};
typedef struct tacc_ident *tacc_ident_p;

struct tacc_token_p {
    pp_tok_p tacc_token_p_content;
};
typedef struct tacc_token_p *tacc_token_pp;

struct tacc_macro_def {
    char *tacc_macro_def_name;

    tacc_token_pp tacc_macro_def_replacement_list;
    int tacc_macro_def_replacement_list_len;
    int tacc_macro_def_is_va;
    int tacc_macro_def_is_tombstone;

    tacc_ident_p tacc_macro_def_params;
    int tacc_macro_def_num_params;
};
typedef struct tacc_macro_def *tacc_macro_def_p;

struct tacc_include_path_entry {
    char *tacc_include_path_entry_content;
};
typedef struct tacc_include_path_entry *tacc_include_path_p;

struct tacc_macro_def_entry {
    tacc_macro_def_p tacc_macro_def_entry_content;
};
typedef struct tacc_macro_def_entry *tacc_macro_def_entry_p;

struct tacc_pp_state {
    tacc_include_path_p tacc_pp_include_path;
    tacc_macro_def_entry_p tacc_pp_macros;
};
typedef struct tacc_pp_state *tacc_pp_state_p;

struct tacc_tok_iter {
    tacc_file_iter_p tacc_tok_iter_file;
    pp_tok_p tacc_tok_iter_pending;
    tacc_pp_state_p tacc_tok_iter_state;

    struct tacc_tok_iter *tacc_tok_iter_override;
};
typedef struct tacc_tok_iter *tacc_tok_iter_p;

tacc_tok_iter_p tacc_tok_iter_new(void);
void tacc_tok_iter_init(tacc_tok_iter_p iter, tacc_file_iter_p file, tacc_pp_state_p state);
pp_tok_p tacc_tok_iter_peek(tacc_tok_iter_p iter);
pp_tok_p tacc_tok_iter_next(tacc_tok_iter_p iter);
tacc_pp_state_p tacc_pp_state_new(void);
void tacc_pp_state_init(tacc_pp_state_p state);
void tacc_pp_define(tacc_pp_state_p state, char *name, char *expansion);
void tacc_pp_undef(tacc_pp_state_p state, char *name);
tacc_macro_def_entry_p tacc_pp_find_macro_or_first_empty(tacc_pp_state_p state, char *name);
void tacc_pp_insert_macro(tacc_pp_state_p state, tacc_macro_def_p macro);
void tacc_pp_add_include_dir(tacc_pp_state_p state, char *dir);

#endif

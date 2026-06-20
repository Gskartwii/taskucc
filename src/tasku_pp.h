#ifndef TASKU_PP_H
#define TASKU_PP_H

#include "dynstring.h"
#include "tasku_file.h"
#include "util.h"

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
    TOK_OTHER,

    TOK_INCDIR_ANGLE,
    TOK_INCDIR_STRING,

    TOK_FAKE_END_OF_MACRO,
    TOK_FAKE_PMARK
};

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

struct tacc_file_iter {
    char *src;
    char *end;
    char *filename;
    tacc_bool is_bol;
    tacc_bool is_ws;
};

struct tacc_file_iter *tacc_file_iter_new(void);
void tacc_file_iter_init(struct tacc_file_iter *iter, struct tacc_file *file);

struct pp_tok {
    enum pp_tok_kind kind;
    enum pp_ident_kind ident_kind;
    struct tacc_string *str;

    tacc_bool preceded_by_ws;
    tacc_bool is_final;
};
char *tacc_pp_to_string(struct pp_tok *tok);
struct pp_tok *tacc_pp_tok_new(void);
void tacc_pp_tok_init(struct pp_tok *tok);
struct pp_tok *tacc_pp_tok_clone(struct pp_tok *tok);

struct tacc_ident {
    struct tacc_string *content;
};

struct tacc_token_p {
    struct pp_tok *content;
};

struct tacc_token_p_list {
    struct tacc_token_p *list;
    size_t list_len;
};

struct tacc_macro_def {
    struct tacc_string *name;

    struct tacc_token_p *replacement_list;
    size_t replacement_list_len;
    tacc_bool is_function_like;
    tacc_bool is_va;
    tacc_bool is_tombstone;

    struct tacc_ident *params;
    size_t num_params;

    tacc_bool is_replacing;
};

struct tacc_include_path_entry {
    struct tacc_string *content;
};

struct tacc_macro_def_entry {
    struct tacc_macro_def *content;
};

struct tacc_pp_state {
    struct tacc_include_path_entry *include_path;
    struct tacc_macro_def_entry *macros;
};

struct tacc_tok_iter {
    struct tacc_file_iter *file_iter;
    struct tacc_pp_state *state;

    struct tacc_token_p *pending;
    size_t pending_len;

    size_t skip_level;
    size_t inc_level;

    tacc_bool in_macro_args;
    tacc_bool in_include_directive;

    struct tacc_tok_iter *override;
};

struct tacc_tok_iter *tacc_tok_iter_new(void);
void tacc_tok_iter_init(struct tacc_tok_iter *iter,
                        struct tacc_file_iter *file,
                        struct tacc_pp_state *state);
struct pp_tok *tacc_tok_iter_peek(struct tacc_tok_iter *iter);
struct pp_tok *tacc_tok_iter_next(struct tacc_tok_iter *iter);
struct tacc_pp_state *tacc_pp_state_new(void);
void tacc_pp_state_init(struct tacc_pp_state *state);
void tacc_pp_define(struct tacc_pp_state *state, char *name, char *expansion);
void tacc_pp_undef(struct tacc_pp_state *state, char *name);
struct tacc_macro_def_entry *
tacc_pp_find_macro_or_first_empty(struct tacc_pp_state *state, char *name);
void tacc_pp_insert_macro(struct tacc_pp_state *state,
                          struct tacc_macro_def *macro);
void tacc_pp_add_include_dir(struct tacc_pp_state *state, char *dir);

#endif

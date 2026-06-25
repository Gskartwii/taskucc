#ifndef TASKU_PP_H
#define TASKU_PP_H

#include "dynarray.h"
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
    /* owning */
    char *orig;
    /* borrow of orig */
    char *src;
    /* borrow of src */
    char *end;
    /* owning, possibly null */
    char *filename;
    tacc_bool is_bol;
    tacc_bool is_ws;
};

/* return: owning */
struct tacc_file_iter *tacc_file_iter_new_file(struct tacc_file *file);
/* iter: owning */
void tacc_file_iter_free(struct tacc_file_iter *iter);

struct pp_tok {
    enum pp_tok_kind kind;
    enum pp_ident_kind ident_kind;
    /* owning */
    struct tacc_string *str;

    tacc_bool preceded_by_ws;
    tacc_bool is_final;
};
/* return: static, tok: borrow */
char *tacc_pp_to_string(struct pp_tok *tok);
/* return: owning */
struct pp_tok *tacc_pp_tok_new(void);
/* tok: borrow */
void tacc_pp_tok_init(struct pp_tok *tok);
/* tok: owned */
void tacc_pp_tok_free(struct pp_tok *tok);
/* return: owning, tok: borrow */
struct pp_tok *tacc_pp_tok_clone(struct pp_tok *tok);

struct tacc_ident {
    /* owning */
    struct tacc_string *content;
};

struct tacc_token_p {
    /* owning */
    struct pp_tok *content;
};

struct tacc_token_p_list {
    /* owning, array of tacc_token_p */
    struct tacc_dynarray *list;
};

struct tacc_ident_list {
    /* owning, array of tacc_ident */
    struct tacc_dynarray *list;
};

struct tacc_macro_def {
    /* owning */
    struct tacc_string *name;

    /* owning */
    struct tacc_token_p_list *replacement_list;
    tacc_bool is_function_like;
    tacc_bool is_va;
    tacc_bool is_tombstone;

    /* owning */
    struct tacc_ident_list *params;

    tacc_bool is_replacing;
};

struct tacc_include_path_entry {
    /* owning */
    struct tacc_string *content;
};

struct tacc_macro_def_entry {
    /* owning */
    struct tacc_macro_def *content;
};

struct tacc_pp_state {
    /* owning */
    struct tacc_include_path_entry *include_path;
    struct tacc_macro_def_entry *macros;
};

struct tacc_tok_iter {
    /* owning */
    struct tacc_file_iter *file_iter;

    /* borrow */
    struct tacc_pp_state *state;

    /* owning */
    struct tacc_token_p_list *pending;

    size_t skip_level;
    size_t inc_level;

    tacc_bool in_macro_args;
    tacc_bool in_include_directive;

    /* owning, possibly null */
    struct tacc_tok_iter *override;
};

/* return: owning */
struct tacc_tok_iter *tacc_tok_iter_new(struct tacc_file_iter *file,
                                        struct tacc_pp_state *state);
/* iter: owning */
void tacc_tok_iter_free(struct tacc_tok_iter *iter);
/* return: borrow, iter: borrow */
struct pp_tok *tacc_tok_iter_peek(struct tacc_tok_iter *iter);
/* return: owning, iter: borrow */
struct pp_tok *tacc_tok_iter_next(struct tacc_tok_iter *iter);
/* return: owning */
struct tacc_pp_state *tacc_pp_state_new(void);
/* state: borrow */
void tacc_pp_state_init(struct tacc_pp_state *state);
/* state: owning */
void tacc_pp_state_free(struct tacc_pp_state *state);
/* state: borrow, name: borrow, expansion: borrow */
void tacc_pp_define(struct tacc_pp_state *state, char *name, char *expansion);
/* state: borrow, name: borrow */
void tacc_pp_undef(struct tacc_pp_state *state, char *name);
/* return: borrow, state: borrow, name: borrow */
struct tacc_macro_def_entry *
tacc_pp_find_macro_or_first_empty(struct tacc_pp_state *state, char *name);
/* state: borrow, macro: owning */
void tacc_pp_insert_macro(struct tacc_pp_state *state,
                          struct tacc_macro_def *macro);
/* state: borrow, dir: owning */
void tacc_pp_add_include_dir(struct tacc_pp_state *state, char *dir);

#endif

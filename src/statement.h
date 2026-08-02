#ifndef TACC_STATEMENT_H
#define TACC_STATEMENT_H

#include "dynarray.h"
#include "util.h"

enum tacc_compound_member_kind { COMPOUND_MEMBER_STMT, COMPOUND_MEMBER_DECL };

struct tacc_compound_member {
    enum tacc_compound_member_kind kind;
    union {
        /* owning */
        struct tacc_statement *statement;

        /* owning */
        struct tacc_decl *declaration;
    } member;
};

enum tacc_statement_kind {
    STMT_NULL,
    STMT_LABEL_NAMED,
    STMT_CASE,
    STMT_DEFAULT,
    STMT_COMPOUND,
    STMT_EXPRESSION,
    STMT_IF,
    STMT_SWITCH,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_GOTO,
    STMT_CONTINUE,
    STMT_BREAK,
    STMT_RETURN
};

struct tacc_label {
    /* owning */
    struct tacc_string *name;
};

struct tacc_if {
    /* owning */
    struct tacc_expr *controlling;

    /* owning */
    struct tacc_statement *then;

    /* owning */
    struct tacc_statement *otherwise;
};

struct tacc_switch_while_do {
    /* owning */
    struct tacc_expr *controlling;

    /* owning */
    struct tacc_statement *body;
};

struct tacc_for {
    tacc_bool init_is_declaration;

    union {
        /* owning */
        struct tacc_decl *declaration;

        /* owning */
        struct tacc_expr *expr;
    } initializer;

    /* owning */
    struct tacc_expr *controlling;

    /* owning */
    struct tacc_expr *after;

    struct tacc_statement *body;
};

struct tacc_statement {
    enum tacc_statement_kind kind;
    union {
        /* STMT_LABEL_NAMED, STMT_GOTO */
        /*
         * Contrary to C99, treat the label as a floating statement rather than
         * a modified on another statement. This allows labels that do not point
         * at any statement; definitely a desired extension.
         *
         * HUGE TODO: does this alter semantics in another meaningful way?
         */
        struct tacc_string *label;

        /* STMT_COMPOUND */
        struct tacc_compound_member_list *sub_statements;

        /* STMT_CASE, STMT_EXPRESSION, STMT_RETURN */
        struct tacc_expr *expr;

        /* STMT_IF */
        struct tacc_if *if_stmt;

        /* STMT_SWITCH, STMT_WHILE, STMT_DO_WHILE */
        struct tacc_switch_while_do *details;

        /* STMT_FOR */
        struct tacc_for *for_statement;
    } extra;
};

struct tacc_compound_member *tacc_compound_member_new(void);
struct tacc_statement *tacc_statement_new(void);
struct tacc_if *tacc_if_new(void);
struct tacc_for *tacc_for_new(void);
struct tacc_switch_while_do *tacc_switch_while_do_new(void);

void tacc_compound_member_free(struct tacc_compound_member *compound_member);
void tacc_for_free(struct tacc_for *for_stmt);
void tacc_if_free(struct tacc_if *if_stmt);
void tacc_switch_while_do_free(
    struct tacc_switch_while_do *switch_while_do_stmt);
void tacc_statement_free(struct tacc_statement *statement);

DECL_DYNARRAY_OVER(tacc_compound_member_list,
                   tacc_compound_member_list_entry,
                   struct tacc_compound_member *,
                   tacc_compound_member_list_new,
                   tacc_compound_member_list_init,
                   tacc_compound_member_list_get,
                   tacc_compound_member_list_push,
                   tacc_compound_member_list_pop,
                   tacc_compound_member_list_len,
                   tacc_compound_member_list_free)

#endif

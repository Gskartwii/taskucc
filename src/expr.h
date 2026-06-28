#ifndef TACC_EXPR_H
#define TACC_EXPR_H

#include "dynarray.h"
#include "dynstring.h"
#include "machine.h"
#include "tasku_pp.h"

enum tacc_expr_kind {
    EX_UNINIT,

    EX_NUM_LIT,
    EX_STRING_LIT,
    EX_IDENT,

    EX_ADD,
    EX_SUB,
    EX_MUL,
    EX_DIV,
    EX_REM,
    EX_POS,
    EX_NEG,

    EX_BAND,
    EX_BOR,
    EX_BXOR,
    EX_BNOT,
    EX_SHL,
    EX_SHR,

    EX_AND,
    EX_OR,
    EX_NOT,

    EX_EQ,
    EX_NE,
    EX_LE,
    EX_LT,
    EX_GE,
    EX_GT,

    EX_ASSI,
    EX_ADD_ASSI,
    EX_SUB_ASSI,
    EX_MUL_ASSI,
    EX_DIV_ASSI,
    EX_REM_ASSI,
    EX_BAND_ASSI,
    EX_BOR_ASSI,
    EX_BXOR_ASSI,
    EX_LSH_ASSI,
    EX_RSH_ASSI,

    EX_INCR_PRE,
    EX_DECR_PRE,
    EX_INCR_POST,
    EX_DECR_POST,

    EX_SUBSCRIPT,
    EX_DEREF,
    EX_ADDROF,
    EX_MEMBER,
    EX_PTR_MEMBER,

    EX_CALL,
    EX_COMMA,
    EX_CAST,
    EX_SIZEOF,
    EX_SIZEOF_TY,
    EX_SELECT,
    EX_COMPOUND_LIT
};

struct tacc_expr;

DECL_DYNARRAY_OVER(tacc_expr_list,
                   tacc_expr_list_entry,
                   struct tacc_expr *,
                   tacc_expr_list_new,
                   tacc_expr_list_init,
                   tacc_expr_list_get,
                   tacc_expr_list_push,
                   tacc_expr_list_pop,
                   tacc_expr_list_len,
                   tacc_expr_list_free)

struct tacc_expr {
    enum tacc_expr_kind kind;

    struct tacc_expr *op1;
    struct tacc_expr *op2;
    struct tacc_expr *op3;

    union {
        struct tacc_expr_list *op_list;
        struct tacc_type *type;
        struct tacc_string *name;
        struct tacc_val *const_val;
    } extra;
};

struct tacc_expr *tacc_expr_parse_new(struct tacc_tok_iter *iter,
                                      tacc_bool in_if);
struct tacc_val *tacc_expr_const_eval(struct tacc_expr *expr);
void tacc_expr_free(struct tacc_expr *expr);

#endif

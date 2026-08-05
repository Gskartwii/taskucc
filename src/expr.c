#include "expr.h"
#include "decl.h"
#include "dynstring.h"
#include "soft_u64.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void tacc_expr_init(struct tacc_expr *expr) {
    expr->op1 = NULL;
    expr->op2 = NULL;
    expr->op3 = NULL;
    expr->kind = EX_UNINIT;
}

struct tacc_expr *tacc_expr_new(void) {
    struct tacc_expr *expr = tacc_malloc(sizeof(struct tacc_expr));
    tacc_expr_init(expr);
    return expr;
}

struct tacc_int_literal *tacc_int_literal_new(void) {
    struct tacc_int_literal *int_literal;

    int_literal = tacc_malloc(sizeof(struct tacc_int_literal));
    int_literal->base = 10;
    int_literal->number = tacc_u64_new();
    int_literal->suffix_l = 0;
    int_literal->suffix_ll = 0;
    int_literal->suffix_u = 0;

    return int_literal;
}

struct tacc_type_name *tacc_type_name_new(void) {
    struct tacc_type_name *type_name =
        tacc_malloc(sizeof(struct tacc_type_name));
    type_name->base_type = NULL;
    type_name->type_extension = NULL;
    return type_name;
}

void tacc_expr_free(struct tacc_expr *expr) {
    if (expr->op1) {
        tacc_expr_free(expr->op1);
    }
    if (expr->op2) {
        tacc_expr_free(expr->op2);
    }
    if (expr->op3) {
        tacc_expr_free(expr->op3);
    }
    if (expr->kind == EX_INT_LIT) {
        tacc_int_literal_free(expr->extra.int_literal);
    } else if (expr->kind == EX_IDENT || expr->kind == EX_MEMBER ||
               expr->kind == EX_PTR_MEMBER) {
        tacc_dynstring_free(expr->extra.name);
    } else if (expr->kind == EX_SIZEOF_TY || expr->kind == EX_CAST) {
        tacc_type_name_free(expr->extra.type);
    } else if (expr->kind == EX_CALL && expr->extra.op_list != NULL) {
        tacc_expr_list_free(expr->extra.op_list);
        tacc_free(expr->extra.op_list);
    }
    tacc_free(expr);
}

void tacc_type_name_free(struct tacc_type_name *ty) {
    tacc_decl_type_free(ty->base_type);
    tacc_declarator_free(ty->type_extension);
    tacc_free(ty);
}

void tacc_int_literal_free(struct tacc_int_literal *literal) {
    tacc_free(literal->number);
    tacc_free(literal);
}

MK_DYNARRAY_OVER(tacc_expr_list,
                 tacc_expr_list_entry,
                 struct tacc_expr *,
                 tacc_expr_list_new,
                 tacc_expr_list_init,
                 tacc_expr_list_get,
                 tacc_expr_list_push,
                 tacc_expr_list_pop,
                 tacc_expr_list_len,
                 tacc_expr_free,
                 tacc_expr_list_free)

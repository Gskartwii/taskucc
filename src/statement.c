#include "statement.h"
#include "decl.h"
#include "util.h"

MK_DYNARRAY_OVER(tacc_compound_member_list,
                 tacc_compound_member_list_entry,
                 struct tacc_compound_member *,
                 tacc_compound_member_list_new,
                 tacc_compound_member_list_init,
                 tacc_compound_member_list_get,
                 tacc_compound_member_list_push,
                 tacc_compound_member_list_pop,
                 tacc_compound_member_list_len,
                 tacc_compound_member_free,
                 tacc_compound_member_list_free)

struct tacc_for *tacc_for_new(void) {
    struct tacc_for *for_stmt;

    for_stmt = tacc_malloc(sizeof(struct tacc_for));
    for_stmt->init_is_declaration = 0;
    for_stmt->initializer.expr = NULL;
    for_stmt->controlling = NULL;
    for_stmt->after = NULL;
    for_stmt->body = NULL;

    return for_stmt;
}

struct tacc_if *tacc_if_new(void) {
    struct tacc_if *if_stmt;

    if_stmt = tacc_malloc(sizeof(struct tacc_if));
    if_stmt->controlling = NULL;
    if_stmt->then = NULL;
    if_stmt->otherwise = NULL;

    return if_stmt;
}

struct tacc_switch_while_do *tacc_switch_while_do_new(void) {
    struct tacc_switch_while_do *switch_while_do_stmt;

    switch_while_do_stmt = tacc_malloc(sizeof(struct tacc_switch_while_do));
    switch_while_do_stmt->controlling = NULL;
    switch_while_do_stmt->body = NULL;

    return switch_while_do_stmt;
}

struct tacc_statement *tacc_statement_new(void) {
    struct tacc_statement *statement;

    statement = tacc_malloc(sizeof(struct tacc_statement));
    statement->kind = STMT_NULL;

    return statement;
}

struct tacc_compound_member *tacc_compound_member_new(void) {
    struct tacc_compound_member *compound_member;

    compound_member = tacc_malloc(sizeof(struct tacc_compound_member));
    compound_member->kind = COMPOUND_MEMBER_STMT;
    compound_member->member.statement = NULL;

    return compound_member;
}

void tacc_compound_member_free(struct tacc_compound_member *member) {
    if (member->kind == COMPOUND_MEMBER_DECL) {
        tacc_decl_free(member->member.declaration);
    } else {
        tacc_statement_free(member->member.statement);
    }
    tacc_free(member);
}

void tacc_for_free(struct tacc_for *for_stmt) {
    if (for_stmt->init_is_declaration) {
        tacc_decl_free(for_stmt->initializer.declaration);
    } else if (for_stmt->initializer.expr != NULL) {
        tacc_expr_free(for_stmt->initializer.expr);
    }
    if (for_stmt->controlling != NULL) {
        tacc_expr_free(for_stmt->controlling);
    }
    if (for_stmt->after != NULL) {
        tacc_expr_free(for_stmt->after);
    }
    tacc_statement_free(for_stmt->body);
    tacc_free(for_stmt);
}

void tacc_if_free(struct tacc_if *if_stmt) {
    tacc_expr_free(if_stmt->controlling);
    tacc_statement_free(if_stmt->then);
    if (if_stmt->otherwise != NULL) {
        tacc_statement_free(if_stmt->otherwise);
    }
    tacc_free(if_stmt);
}
void tacc_switch_while_do_free(
    struct tacc_switch_while_do *switch_while_do_stmt) {
    tacc_expr_free(switch_while_do_stmt->controlling);
    tacc_statement_free(switch_while_do_stmt->body);
    tacc_free(switch_while_do_stmt);
}

void tacc_statement_free(struct tacc_statement *statement) {
    switch (statement->kind) {
    case STMT_NULL:
    case STMT_DEFAULT:
    case STMT_CONTINUE:
    case STMT_BREAK:
        break;

    case STMT_LABEL_NAMED:
    case STMT_GOTO:
        tacc_dynstring_free(statement->extra.label);
        break;

    case STMT_CASE:
    case STMT_EXPRESSION:
    case STMT_RETURN:
        tacc_expr_free(statement->extra.expr);
        break;

    case STMT_COMPOUND:
        tacc_compound_member_list_free(statement->extra.sub_statements);
        tacc_free(statement->extra.sub_statements);
        break;

    case STMT_IF:
        tacc_if_free(statement->extra.if_stmt);
        break;

    case STMT_SWITCH:
    case STMT_WHILE:
    case STMT_DO_WHILE:
        tacc_switch_while_do_free(statement->extra.details);
        break;

    case STMT_FOR:
        tacc_for_free(statement->extra.for_statement);
        break;
    }
    tacc_free(statement);
}

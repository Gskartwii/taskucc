#include "codegen.h"
#include "dynstring.h"
#include "expr.h"
#include "machine.h"
#include "statement.h"
#include "target/codegen.h"
#include "type.h"

MK_DYNARRAY_OVER(tacc_slot_list,
                 tacc_slot_list_entry,
                 struct tacc_slot *,
                 tacc_slot_list_new,
                 tacc_slot_list_init,
                 tacc_slot_list_get,
                 tacc_slot_list_push,
                 tacc_slot_list_pop,
                 tacc_slot_list_len,
                 tacc_slot_free,
                 tacc_slot_list_free)

void tacc_codegen_output(struct tacc_codegen_state *state, char *fmt, ...) {
    va_list va;

    va_start(va, fmt);
    tacc_dynstring_vprintf(state->code_buffer, fmt, va);
    va_end(va);
}

struct tacc_codegen_state *tacc_codegen_state_new(
    struct tacc_target *target, struct tacc_type_list *basic_types) {
    struct tacc_codegen_state *state;

    state = tacc_malloc(sizeof(struct tacc_codegen_state));
    state->target_state = tacc_target_codegen_state_new();
    state->target = target;
    state->basic_types = basic_types;
    state->code_buffer = tacc_dynstring_new();
    state->stack = tacc_slot_list_new();

    return state;
}

void tacc_codegen_compile_expr(struct tacc_codegen_state *state,
                               struct tacc_expr *expr) {
    switch (expr->kind) {
    case EX_UNINIT:
    case EX_INT_LIT:
        tacc_target_codegen_int(
            state,
            tacc_expr_const_eval(expr, state->target, state->basic_types));
        break;
    case EX_CHAR_LIT:
    case EX_STRING_LIT:
    case EX_IDENT:
    case EX_ADD:
    case EX_SUB:
    case EX_MUL:
    case EX_DIV:
    case EX_REM:
    case EX_POS:
    case EX_NEG:
    case EX_BAND:
    case EX_BOR:
    case EX_BXOR:
    case EX_BNOT:
    case EX_SHL:
    case EX_SHR:
    case EX_AND:
    case EX_OR:
    case EX_NOT:
    case EX_EQ:
    case EX_NE:
    case EX_LE:
    case EX_LT:
    case EX_GE:
    case EX_GT:
    case EX_ASSI:
    case EX_ADD_ASSI:
    case EX_SUB_ASSI:
    case EX_MUL_ASSI:
    case EX_DIV_ASSI:
    case EX_REM_ASSI:
    case EX_BAND_ASSI:
    case EX_BOR_ASSI:
    case EX_BXOR_ASSI:
    case EX_LSH_ASSI:
    case EX_RSH_ASSI:
    case EX_INCR_PRE:
    case EX_DECR_PRE:
    case EX_INCR_POST:
    case EX_DECR_POST:
    case EX_SUBSCRIPT:
    case EX_DEREF:
    case EX_ADDROF:
    case EX_MEMBER:
    case EX_PTR_MEMBER:
    case EX_CALL:
    case EX_COMMA:
    case EX_CAST:
    case EX_SIZEOF:
    case EX_SIZEOF_TY:
    case EX_SELECT:
    case EX_COMPOUND_LIT:
        tacc_assert(0, "TODO: unsupported expression in codegen");
        break;
    }
}

static tacc_bool tacc_codegen_top_is_int(struct tacc_codegen_state *state) {
    struct tacc_slot *slot;

    slot = tacc_codegen_get_top(state);

    return tacc_type_is_integral(slot->ty);
}

void tacc_codegen_compile_body_member(struct tacc_codegen_state *state,
                                      struct tacc_compound_member *member) {
    tacc_assert(member->kind == COMPOUND_MEMBER_STMT,
                "TODO: compile declaration in body");
    switch (member->member.statement->kind) {
    case STMT_NULL:
        break;
    case STMT_RETURN:
        tacc_codegen_compile_expr(state, member->member.statement->extra.expr);
        tacc_assert(tacc_codegen_top_is_int(state),
                    "TODO: return of non-integral type");
        tacc_target_codegen_return_top_int(state);
        break;
    case STMT_LABEL_NAMED:
    case STMT_CASE:
    case STMT_DEFAULT:
    case STMT_COMPOUND:
    case STMT_EXPRESSION:
    case STMT_IF:
    case STMT_SWITCH:
    case STMT_WHILE:
    case STMT_DO_WHILE:
    case STMT_FOR:
    case STMT_GOTO:
    case STMT_CONTINUE:
    case STMT_BREAK:
        tacc_assert(0, "TODO: unsupported statement in codegen");
        break;
    }
}

void tacc_codegen_compile_statements(
    struct tacc_codegen_state *state,
    struct tacc_compound_member_list *statements) {
    size_t i;
    struct tacc_compound_member_list_entry *entry;

    for (i = 0; i < tacc_compound_member_list_len(statements); i = i + 1) {
        entry = tacc_compound_member_list_get(statements, i);
        tacc_codegen_compile_body_member(state, entry->content);
    }
}

void tacc_codegen_slot_spill(struct tacc_codegen_state *state,
                             struct tacc_slot *slot) {
#ifndef __M2__
    (void) state;
    (void) slot;
#endif
    tacc_assert(0, "TODO: spill");
}

struct tacc_slot *tacc_codegen_get_top(struct tacc_codegen_state *state) {
    struct tacc_slot_list_entry *entry;

    entry =
        tacc_slot_list_get(state->stack, tacc_slot_list_len(state->stack) - 1);

    return entry->content;
}

void tacc_codegen_pop(struct tacc_codegen_state *state) {
    tacc_slot_list_pop(state->stack);
}

void tacc_slot_free(struct tacc_slot *slot) {
    /* TODO: free */
#ifndef __M2__
    (void) slot;
#endif
}

struct tacc_slot *tacc_slot_new(void) {
    struct tacc_slot *slot;

    slot = tacc_malloc(sizeof(struct tacc_slot));
    slot->place_kind = PLACE_REGISTER;
    slot->place.reg = NULL;
    slot->ty = NULL;

    return slot;
}

void tacc_codegen_push_reg(struct tacc_codegen_state *state,
                           struct tacc_target_place_register *reg,
                           struct tacc_type *ty) {
    struct tacc_slot *slot;

    slot = tacc_slot_new();
    slot->place_kind = PLACE_REGISTER;
    slot->place.reg = reg;
    slot->ty = ty;

    tacc_slot_list_push(state->stack, slot);
}

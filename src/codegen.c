#include "codegen.h"
#include "call_itf.h"
#include "compile.h"
#include "decl.h"
#include "dynstring.h"
#include "expr.h"
#include "machine.h"
#include "statement.h"
#include "target/call_itf.h"
#include "target/codegen.h"
#include "target/target.h"
#include "type.h"
#include "util.h"

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

MK_DYNHASH_OVER_U32(tacc_local_var_map,
                    name_ref,
                    tacc_local_var_map_entry,
                    struct tacc_local_var *,
                    tacc_local_var_map_new,
                    tacc_local_var_map_init,
                    tacc_local_var_map_get,
                    tacc_local_var_map_insert,
                    tacc_local_var_map_fill_count,
                    tacc_local_var_free,
                    tacc_local_var_map_free)

void tacc_cg_output(struct tacc_cg_state *state, char *fmt, ...) {
    va_list va;

    va_start(va, fmt);
    tacc_dynstring_vprintf(state->code_buffer, fmt, va);
    va_end(va);
}

void tacc_cg_output_prelude(struct tacc_cg_state *state, char *fmt, ...) {
    va_list va;

    va_start(va, fmt);
    tacc_dynstring_vprintf(state->prelude_buffer, fmt, va);
    va_end(va);
}

struct tacc_cg_state *tacc_cg_state_new(struct tacc_compiler *compiler) {
    struct tacc_cg_state *state;

    state = tacc_malloc(sizeof(struct tacc_cg_state));
    state->target_state = tacc_target_cg_state_new();
    state->compiler = compiler;
    state->code_buffer = tacc_dynstring_new();
    state->prelude_buffer = tacc_dynstring_new();
    state->stack = tacc_slot_list_new();
    state->param_names = tacc_ident_list_new();
    state->locals = tacc_local_var_map_new(0x100);
    state->num_local_bytes = 0;
    state->clobbered_registers = 0;
    state->func_type = NULL;

    return state;
}

struct tacc_local_var *tacc_cg_resolve_local(struct tacc_cg_state *state,
                                             uint32_t name_ref) {
    struct tacc_local_var_map_entry *entry;

    entry = tacc_local_var_map_get(state->locals, name_ref);
    if (entry == NULL) {
        return NULL;
    }
    return entry->content;
}

static void tacc_cg_compile_lval(struct tacc_cg_state *state,
                                 struct tacc_expr *expr) {
    struct tacc_local_var *var;

    switch (expr->kind) {
    case EX_UNINIT:
    case EX_INT_LIT:
    case EX_CHAR_LIT:
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
    case EX_ADDROF:
    case EX_CALL:
    case EX_CAST:
    case EX_SIZEOF:
    case EX_SIZEOF_TY:
    case EX_COMMA:
    case EX_SELECT:
        tacc_assert(ASSERT_DIAG, 0, "invalid lvalue");
        break;

    case EX_IDENT:
        var = tacc_cg_resolve_local(state, expr->extra.name_ref);
        if (var != NULL) {
            tacc_assert(ASSERT_TODO,
                        tacc_type_is_integral(var->ty),
                        "load non-integral value");
            tacc_target_cg_addrof_var(state, var);
        } else {
            tacc_assert(ASSERT_TODO, 0, "resolve non-local names");
        }
        break;

    case EX_STRING_LIT:
    case EX_INCR_PRE:
    case EX_DECR_PRE:
    case EX_INCR_POST:
    case EX_DECR_POST:
    case EX_SUBSCRIPT:
    case EX_DEREF:
    case EX_MEMBER:
    case EX_PTR_MEMBER:
    case EX_COMPOUND_LIT:
    case EX_NAME_OF_FUNC:
        tacc_assert(ASSERT_TODO, 0, "lvalue");
        break;
    }
}

void tacc_cg_deref(struct tacc_cg_state *state) {
    struct tacc_slot *slot;
    struct tacc_type *pointed_ty;

    slot = tacc_cg_get_top(state);
    tacc_assert(ASSERT_DIAG,
                slot->ty->kind == TYK_PTR,
                "attempt to dereference non-pointer");
    pointed_ty = slot->ty->extra.pointer.pointee;
    tacc_assert(
        ASSERT_TODO, tacc_type_is_integral(pointed_ty), "load of non-integer");
    tacc_target_cg_deref_int(state, pointed_ty);
}

void tacc_cg_compile_expr(struct tacc_cg_state *state, struct tacc_expr *expr) {
    struct tacc_val *val;
    struct tacc_slot *slot;

    switch (expr->kind) {
    case EX_INT_LIT:
        val = tacc_expr_const_eval(
            expr, state->compiler->target, state->compiler->basic_types);
        tacc_target_cg_int(state, val);
        slot = tacc_cg_get_top(state);
        slot->ty = val->type;
        tacc_val_free(val);
        break;

    case EX_IDENT:
        tacc_cg_compile_lval(state, expr);
        tacc_cg_deref(state);
        break;

    case EX_UNINIT:
    case EX_ASSI:
    case EX_CHAR_LIT:
    case EX_STRING_LIT:
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
    case EX_NAME_OF_FUNC:
        tacc_assert(ASSERT_TODO, 0, "unsupported expression in codegen");
        break;
    }
}

static tacc_bool tacc_cg_top_is_int(struct tacc_cg_state *state) {
    struct tacc_slot *slot;

    slot = tacc_cg_get_top(state);

    return tacc_type_is_integral(slot->ty);
}

void tacc_cg_convert_top(struct tacc_cg_state *state,
                         struct tacc_type *to_type) {
    struct tacc_slot *slot;
    struct tacc_type *from_type;

    slot = tacc_cg_get_top(state);
    from_type = slot->ty;
    if (tacc_int_type_has_compatible_repr(to_type, from_type)) {
        return;
    }
    if (tacc_type_bit_width(from_type) > tacc_type_bit_width(to_type)) {
        tacc_target_cg_narrow_top(
            state, to_type, tacc_type_kind_is_signed(to_type->kind));
        slot->ty = to_type;
        return;
    }
    tacc_target_cg_ext_top(state,
                           to_type,
                           tacc_type_kind_is_signed(to_type->kind) &&
                               tacc_type_kind_is_signed(from_type->kind));
    slot = tacc_cg_get_top(state);
    slot->ty = to_type;
}

static void tacc_cg_decl(struct tacc_cg_state *state, struct tacc_decl *decl) {
    struct tacc_init_declarator_list_entry *entry;
    struct tacc_type *base_ty;
    struct tacc_type *derived_ty;
    struct tacc_local_var *allocated_var;
    size_t i;

    tacc_assert(ASSERT_DIAG,
                decl->kind != DECL_FUNCTION_DEF,
                "function definition within function body");
    if (decl->storage_class == STORAGE_TYPEDEF) {
        tacc_assert(ASSERT_TODO, 0, "typedef in function body");
        return;
    }

    tacc_assert(ASSERT_TODO,
                decl->storage_class == STORAGE_AUTO ||
                    decl->storage_class == STORAGE_REGISTER ||
                    decl->storage_class == STORAGE_UNSPECIFIED,
                "non-local storage class in function");

    base_ty = tacc_type_from_decl_type(state->compiler, decl->base_type);
    for (i = 0; i < tacc_init_declarator_list_len(decl->extra.declarators);
         i = i + 1) {
        entry = tacc_init_declarator_list_get(decl->extra.declarators, i);
        derived_ty = tacc_type_adjust_from_declarator(
            state->compiler, base_ty, entry->content->declarator);
        /* TODO: evaluate remaining VLA sizes */
        allocated_var = tacc_cg_alloc_variable(
            state,
            derived_ty,
            tacc_declarator_name(entry->content->declarator));

        tacc_assert(ASSERT_TODO,
                    entry->content->initializer == NULL,
                    "initializers in function bodies");
        TACC_UNUSED(allocated_var);
    }
}

void tacc_cg_compile_body_member(struct tacc_cg_state *state,
                                 struct tacc_compound_member *member) {
    if (member->kind == COMPOUND_MEMBER_DECL) {
        tacc_cg_decl(state, member->member.declaration);
        return;
    }
    switch (member->member.statement->kind) {
    case STMT_NULL:
        break;
    case STMT_RETURN:
        tacc_cg_compile_expr(state, member->member.statement->extra.expr);
        tacc_cg_convert_top(state, state->func_type->return_type);
        tacc_assert(ASSERT_TODO,
                    tacc_cg_top_is_int(state),
                    "return of non-integral type");
        tacc_target_cg_return_top_int(state);
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
        tacc_assert(ASSERT_TODO, 0, "unsupported statement in codegen");
        break;
    }
}

void tacc_cg_compile_function(struct tacc_cg_state *state,
                              struct tacc_funcdef *func_def,
                              struct tacc_function_type *func_type) {
    size_t i;
    uint32_t param_name;
    struct tacc_compound_member_list_entry *entry;
    struct tacc_declarator *declarator;
    struct tacc_function_param_list *param_list;
    struct tacc_function_param_list_entry *param_entry;
    struct tacc_type_list_entry *param_type_entry;
    struct tacc_callitf_part_list_entry *itf_part_entry;

    state->func_type = func_type;
    state->interface = tacc_target_callitf_from_func_type(func_type);
    state->func_name = tacc_declarator_name(func_def->func_declaration);
    state->num_local_bytes = (size_t) (state->interface->frame_offset);

    tacc_assert(ASSERT_TODO,
                !state->func_type->is_vararg,
                "support vararg in compile_statements");
    declarator = func_def->innermost_declarator;
    if (declarator->extra.func_decl->param_list_kind == FUNCPARAM_LIST) {
        param_list = declarator->extra.func_decl->param_list.modern_params;
        for (i = 0;
             i < tacc_callitf_part_list_len(state->interface->param_parts);
             i = i + 1) {
            itf_part_entry =
                tacc_callitf_part_list_get(state->interface->param_parts, i);
            if (itf_part_entry->content->offset_from_param_start != 0) {
                /* this stack variable allocated earlier */
                continue;
            }

            param_entry = tacc_function_param_list_get(
                param_list, itf_part_entry->content->param_idx);
            param_type_entry = tacc_type_list_get(
                func_type->param_types, itf_part_entry->content->param_idx);
            param_name = tacc_declarator_name(param_entry->content->decl);

            if (itf_part_entry->content->place.kind == CALLITF_PLACE_STACK) {
                tacc_cg_add_variable(
                    state,
                    param_type_entry->content,
                    param_name,
                    itf_part_entry->content->place.extra.stack_offset);
            } else {
                tacc_cg_alloc_variable(
                    state, param_type_entry->content, param_name);
            }

            tacc_ident_list_push(state->param_names, param_name);
        }
    }

    for (i = 0; i < tacc_compound_member_list_len(func_def->statements);
         i = i + 1) {
        entry = tacc_compound_member_list_get(func_def->statements, i);
        tacc_cg_compile_body_member(state, entry->content);
    }
}

void tacc_cg_slot_spill(struct tacc_cg_state *state, struct tacc_slot *slot) {
    TACC_UNUSED(state);
    TACC_UNUSED(slot);
    tacc_assert(ASSERT_TODO, 0, "spill");
}

struct tacc_slot *tacc_cg_get_top(struct tacc_cg_state *state) {
    struct tacc_slot_list_entry *entry;

    entry =
        tacc_slot_list_get(state->stack, tacc_slot_list_len(state->stack) - 1);

    return entry->content;
}

void tacc_cg_pop(struct tacc_cg_state *state) {
    tacc_slot_free(tacc_slot_list_pop(state->stack));
}

void tacc_slot_free(struct tacc_slot *slot) {
    if (slot->place_kind == PLACE_REGISTER) {
        tacc_target_place_register_free(slot->place.reg);
    }
    tacc_free(slot);
}

struct tacc_slot *tacc_slot_new(void) {
    struct tacc_slot *slot;

    slot = tacc_malloc(sizeof(struct tacc_slot));
    slot->place_kind = PLACE_REGISTER;
    slot->place.reg = NULL;
    slot->ty = NULL;

    return slot;
}

void tacc_cg_push_reg(struct tacc_cg_state *state,
                      struct tacc_target_place_register *reg,
                      struct tacc_type *ty) {
    struct tacc_slot *slot;

    slot = tacc_slot_new();
    slot->place_kind = PLACE_REGISTER;
    slot->place.reg = reg;
    slot->ty = ty;

    tacc_slot_list_push(state->stack, slot);
}
void tacc_cg_push_reg_pair(struct tacc_cg_state *state,
                           struct tacc_target_place_register *reg,
                           struct tacc_target_place_register *reg_2,
                           struct tacc_type *ty) {
    struct tacc_slot *slot;

    slot = tacc_slot_new();
    slot->place_kind = PLACE_REGISTER_PAIR;
    slot->place.pair.reg = reg;
    slot->place.pair.reg_2 = reg_2;
    slot->ty = ty;

    tacc_slot_list_push(state->stack, slot);
}

uint32_t tacc_target_cg_alloc_reg(struct tacc_cg_state *state,
                                  uint32_t desired_registers) {
    size_t i;
    size_t oldest_matching;
    tacc_bool found_matching;
    uint32_t occupied_registers;
    uint32_t available;
    struct tacc_slot_list_entry *slot_entry;
    uint32_t reg_chosen;

    /* steal slot from oldest stack entry that uses a desirable register */
    found_matching = 0;
    occupied_registers = 0;
    oldest_matching = 0;
    for (i = 0; i < tacc_slot_list_len(state->stack); i = i + 1) {
        slot_entry = tacc_slot_list_get(state->stack, i);
        if (slot_entry->content->place_kind == PLACE_REGISTER ||
            slot_entry->content->place_kind == PLACE_REGISTER_PAIR) {
            if ((slot_entry->content->place.reg->reg & desired_registers) !=
                0) {
                if (!found_matching) {
                    found_matching = 1;
                    oldest_matching = i;
                }
                occupied_registers =
                    occupied_registers | slot_entry->content->place.reg->reg;
            }
        }
    }
    if (occupied_registers == desired_registers) {
        slot_entry = tacc_slot_list_get(state->stack, oldest_matching);
        reg_chosen = slot_entry->content->place.reg->reg;
        tacc_cg_slot_spill(state, slot_entry->content);
    } else {
        available = desired_registers & ~(occupied_registers);
        reg_chosen = available & (-available);
    }
    state->clobbered_registers = state->clobbered_registers | reg_chosen;
    return reg_chosen;
}

struct tacc_target_place_register *tacc_target_place_register_new(void) {
    struct tacc_target_place_register *reg;

    reg = tacc_malloc(sizeof(struct tacc_target_place_register));
    reg->reg = 0;

    return reg;
}

void tacc_target_place_register_free(struct tacc_target_place_register *reg) {
    tacc_free(reg);
}

void tacc_cg_state_free(struct tacc_cg_state *state) {
    tacc_target_cg_state_free(state->target_state);
    tacc_dynstring_free(state->code_buffer);
    tacc_dynstring_free(state->prelude_buffer);
    tacc_ident_list_free(state->param_names);
    tacc_free(state->param_names);
    tacc_local_var_map_free(state->locals);
    tacc_free(state->locals);
    tacc_slot_list_free(state->stack);
    tacc_free(state->stack);
    tacc_callitf_free(state->interface);

    tacc_free(state);
}

void tacc_cg_move(struct tacc_cg_state *state,
                  struct tacc_slot *slot,
                  uint32_t permissible_regs) {
    uint32_t new_reg;

    if (slot->place_kind == PLACE_REGISTER &&
        (slot->place.reg->reg & permissible_regs) != 0) {
        return;
    }
    if (slot->place_kind == PLACE_REGISTER) {
        new_reg = tacc_target_cg_alloc_reg(state, permissible_regs);
        tacc_target_cg_move_reg_reg(state, slot->place.reg->reg, new_reg);
        slot->place.reg->reg = new_reg;
    } else {
        tacc_assert(ASSERT_TODO, 0, "move from stack to register");
    }
}

void tacc_cg_move_pair(struct tacc_cg_state *state,
                       struct tacc_slot *slot,
                       uint32_t permissible_low,
                       uint32_t permissible_high) {
    uint32_t new_reg;
    uint32_t old_low;
    uint32_t old_high;

    if (slot->place_kind == PLACE_REGISTER_PAIR) {
        old_low = slot->place.pair.reg->reg;
        old_high = slot->place.pair.reg_2->reg;
        if ((old_low & permissible_low) != 0 &&
            (old_high & permissible_high) != 0) {
            return;
        }

        if ((old_low & permissible_high) != 0) {
            tacc_target_cg_xchg_reg_reg(state, old_low, old_high);

            slot->place.pair.reg_2->reg = old_low;
            slot->place.pair.reg->reg = old_high;
            permissible_low = permissible_low & ~old_low;
            if ((old_high & permissible_low) != 0) {
                /* exchange was sufficient to fix the pair. we're done. */
                slot->place.pair.reg->reg = old_high;
                return;
            }

            /* high is ok. find a new register for low word */
            new_reg = tacc_target_cg_alloc_reg(state, permissible_low);
            tacc_target_cg_move_reg_reg(
                state, slot->place.pair.reg->reg, new_reg);
            slot->place.pair.reg->reg = new_reg;
            return;
        }
        if ((old_high & permissible_low) != 0) {
            tacc_target_cg_xchg_reg_reg(state, old_low, old_high);

            slot->place.pair.reg_2->reg = old_low;
            slot->place.pair.reg->reg = old_high;
            permissible_high = permissible_high & ~old_high;

            /*
             * no need to check if pair was fixed by xchg. that would
             * have been caught by the other condition if.
             */

            /* low is ok. find a new register for high word. */
            new_reg = tacc_target_cg_alloc_reg(state, permissible_high);
            tacc_target_cg_move_reg_reg(
                state, slot->place.pair.reg_2->reg, new_reg);
            slot->place.pair.reg_2->reg = new_reg;
            return;
        }

        /*
         * The registers currently occupied by this pair are not desirable for
         * this pair (among those in `permissible_high|permissible_low`).
         * Therefore allocating two new registers will not clash with either
         * part of this pair.
         */
        new_reg = tacc_target_cg_alloc_reg(state, permissible_high);
        tacc_target_cg_move_reg_reg(
            state, slot->place.pair.reg_2->reg, new_reg);
        new_reg = tacc_target_cg_alloc_reg(state, permissible_low & ~new_reg);
        tacc_target_cg_move_reg_reg(state, slot->place.pair.reg->reg, new_reg);
    } else {
        tacc_assert(ASSERT_TODO, 0, "move from stack to register pair");
    }
}

void tacc_cg_ensure_top_is_pair(struct tacc_cg_state *state,
                                uint32_t *lo_reg,
                                uint32_t *hi_reg) {
    struct tacc_slot *slot;

    slot = tacc_cg_get_top(state);

    tacc_assert(ASSERT_TODO,
                slot->place_kind == PLACE_REGISTER_PAIR,
                "expected register pair at stack top");
    *lo_reg = slot->place.pair.reg->reg;
    *hi_reg = slot->place.pair.reg_2->reg;
}
uint32_t tacc_cg_ensure_top_is_single(struct tacc_cg_state *state) {
    struct tacc_slot *slot;

    slot = tacc_cg_get_top(state);

    tacc_assert(ASSERT_TODO,
                slot->place_kind == PLACE_REGISTER,
                "expected register at stack top");
    return slot->place.reg->reg;
}

void tacc_cg_finalize(struct tacc_cg_state *state) {
    tacc_cg_output(state, "\n.L%u_epilog:", state->func_name);
    tacc_target_cg_finalize(state);
}

struct tacc_local_var *tacc_cg_alloc_variable(struct tacc_cg_state *state,
                                              struct tacc_type *ty,
                                              uint32_t name_ref) {
    struct tacc_local_var *var;
    size_t size;
    size_t align;

    size = tacc_type_size(ty);
    align = tacc_type_alignment_p2(ty);
    tacc_assert(ASSERT_DIAG,
                align <= 4,
                "type alignment %d exceeds stack alignment of 16",
                align);
    state->num_local_bytes = tacc_align_up(state->num_local_bytes, align);
    state->num_local_bytes = state->num_local_bytes + size;
    var = tacc_cg_add_variable(
        state, ty, name_ref, -((int) (state->num_local_bytes)));

    return var;
}

struct tacc_local_var *tacc_cg_add_variable(struct tacc_cg_state *state,
                                            struct tacc_type *ty,
                                            uint32_t name_ref,
                                            int stack_offset) {
    struct tacc_local_var *var;

    var = tacc_local_var_new();
    var->name_ref = name_ref;
    var->offset = stack_offset;
    var->ty = ty;
    tacc_local_var_map_insert(state->locals, var);

    return var;
}

struct tacc_local_var *tacc_local_var_new(void) {
    struct tacc_local_var *var;

    var = tacc_malloc(sizeof(struct tacc_local_var));
    var->name_ref = 0;
    var->ty = NULL;
    var->offset = 0;

    return var;
}

void tacc_local_var_free(struct tacc_local_var *var) { tacc_free(var); }

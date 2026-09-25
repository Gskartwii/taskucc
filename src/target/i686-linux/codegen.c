#include "codegen.h"
#include "compile.h"
#include "machine.h"
#include "target/codegen.h"
#include "target/i686-linux/registers.h"
#include "target/target.h"
#include "type.h"
#include "util.h"

struct tacc_target_cg_state {
    int x;
};

struct tacc_target_cg_state *tacc_target_cg_state_new(void) {
    struct tacc_target_cg_state *state;

    state = tacc_malloc(sizeof(struct tacc_target_cg_state));

    return state;
}

static char *tacc_target_register_as_32(enum tacc_target_register reg) {
    switch (reg) {
    case REG_EAX:
        return "%eax";
    case REG_EBX:
        return "%ebx";
    case REG_ECX:
        return "%ecx";
    case REG_EDX:
        return "%edx";
    case REG_ESI:
        return "%esi";
    case REG_EDI:
        return "%edi";
    default:
        return "INVALID-32";
    }
}

static char *tacc_target_register_as_16(enum tacc_target_register reg) {
    switch (reg) {
    case REG_EAX:
        return "%ax";
    case REG_EBX:
        return "%bx";
    case REG_ECX:
        return "%cx";
    case REG_EDX:
        return "%dx";
    case REG_ESI:
        return "%si";
    case REG_EDI:
        return "%di";
    default:
        return "INVALID-16";
    }
}

static char *tacc_target_register_as_8(enum tacc_target_register reg) {
    switch (reg) {
    case REG_EAX:
        return "%al";
    case REG_EBX:
        return "%bl";
    case REG_ECX:
        return "%cl";
    case REG_EDX:
        return "%dl";
    case REG_ESI:
        return "%sil";
    case REG_EDI:
        return "%dil";
    default:
        return "INVALID-8";
    }
}

static char *tacc_target_register_name(enum tacc_target_register reg,
                                       size_t width) {
    switch (width) {
    case 32:
        return tacc_target_register_as_32(reg);
    case 16:
        return tacc_target_register_as_16(reg);
    case 8:
        return tacc_target_register_as_8(reg);
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid width %d", width);
        return NULL;
    }
}

static char *tacc_target_op_suffix(size_t width) {
    switch (width) {
    case 32:
        return "l";
    case 16:
        return "w";
    case 8:
        return "b";
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid width %d", width);
        return NULL;
    }
}

void tacc_target_cg_int_pair(struct tacc_cg_state *state,
                             struct tacc_val *val) {
    enum tacc_target_register register_place;
    enum tacc_target_register register_place_2;
    struct tacc_target_place_register *reg_place;
    struct tacc_target_place_register *reg_place_2;
    char *reg;

    register_place = tacc_target_cg_alloc_reg(state, REG_ANY);
    register_place_2 =
        tacc_target_cg_alloc_reg(state, REG_ANY & ~(register_place));

    reg = tacc_target_register_as_32(register_place);
    tacc_cg_output(
        state, "\n\t movl $0x%x, %s", val->value.int_value->low, reg);
    reg = tacc_target_register_as_32(register_place_2);
    tacc_cg_output(
        state, "\n\t movl $0x%x, %s", val->value.int_value->high, reg);

    reg_place = tacc_target_place_register_new();
    reg_place->reg = register_place;
    reg_place_2 = tacc_target_place_register_new();
    reg_place_2->reg = register_place_2;
    tacc_cg_push_reg_pair(state, reg_place, reg_place_2, val->type);
}

void tacc_target_cg_move_reg_reg(struct tacc_cg_state *state,
                                 uint32_t from_reg,
                                 uint32_t to_reg) {
    tacc_cg_output(state,
                   "\n\t movl %s, %s",
                   tacc_target_register_as_32(from_reg),
                   tacc_target_register_as_32(to_reg));
}

void tacc_target_cg_int(struct tacc_cg_state *state, struct tacc_val *val) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    char *reg;
    size_t width;

    width = tacc_type_bit_width(val->type);

    if (width > 32) {
        tacc_target_cg_int_pair(state, val);
        return;
    }

    register_place = tacc_target_cg_alloc_reg(state, REG_ANY);

    reg = tacc_target_register_name(register_place, width);
    tacc_cg_output(state,
                   "\n\t mov%s $0x%x, %s",
                   tacc_target_op_suffix(width),
                   val->value.int_value->low,
                   reg);

    reg_place = tacc_target_place_register_new();
    reg_place->reg = register_place;
    tacc_cg_push_reg(state, reg_place, val->type);
}

void tacc_target_cg_return_top_int(struct tacc_cg_state *state) {
    size_t width;
    struct tacc_slot *slot;

    slot = tacc_cg_get_top(state);
    width = tacc_type_bit_width(slot->ty);
    if (width > 32) {
        tacc_cg_move_pair(state, tacc_cg_get_top(state), REG_EAX, REG_EDX);
    } else {
        tacc_cg_move(state, tacc_cg_get_top(state), REG_EAX);
    }
    tacc_cg_pop(state);
    tacc_cg_output(state, "\n\t jmp .L%u_epilog", state->func_name);
}

void tacc_target_cg_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

void tacc_target_cg_state_free(struct tacc_target_cg_state *state) {
    tacc_free(state);
}

void tacc_target_cg_narrow_top(struct tacc_cg_state *state,
                               struct tacc_type *to_type,
                               tacc_bool is_sext) {
    struct tacc_slot *slot;
    struct tacc_target_place_register *top_place;
    size_t from_width;
    size_t to_width;

    slot = tacc_cg_get_top(state);
    from_width = tacc_type_bit_width(slot->ty);
    to_width = tacc_type_bit_width(to_type);

    if (from_width > 32 && to_width <= 32) {
        /* first cast to int and forget reg_2 */
        top_place = slot->place.pair.reg;
        slot->place_kind = PLACE_REGISTER;
        slot->place.reg = top_place;
        if (tacc_type_kind_is_signed(slot->ty->kind)) {
            slot->ty =
                tacc_get_basic_type(state->compiler->basic_types, TYK_SINT);
        } else {
            slot->ty =
                tacc_get_basic_type(state->compiler->basic_types, TYK_UINT);
        }
    }

    /* leave high bits indeterminate */
    TACC_UNUSED(is_sext);
}

void tacc_target_cg_ext_top(struct tacc_cg_state *state,
                            struct tacc_type *type,
                            tacc_bool is_sext) {
    struct tacc_slot *slot;
    uint32_t top_place;
    enum tacc_target_register top_reg;
    enum tacc_target_register top_reg_2;
    size_t from_width;
    size_t to_width;
    char *reg_name;
    char *reg_name_2;
    char *op_base;
    char *op_suff_from;
    char *op_suff_to;

    slot = tacc_cg_get_top(state);
    from_width = tacc_type_bit_width(slot->ty);
    to_width = tacc_type_bit_width(type);

    if (from_width == to_width) {
        /* Plain sign-conversion. */
        return;
    }

    if (from_width <= 32 && to_width > 32) {
        tacc_target_cg_ext_top(
            state,
            tacc_get_basic_type(state->compiler->basic_types, TYK_UINT),
            is_sext);
        slot = tacc_cg_get_top(state);
        top_place = tacc_cg_ensure_top_is_single(state, REG_ANY);
        top_reg_2 = tacc_target_cg_alloc_reg(state, REG_ANY);
        slot->place_kind = PLACE_REGISTER_PAIR;
        slot->place.pair.reg = tacc_target_place_register_new();
        slot->place.pair.reg->reg = top_place;
        slot->place.pair.reg_2 = tacc_target_place_register_new();
        slot->place.pair.reg_2->reg = top_reg_2;
        reg_name = tacc_target_register_name(top_place, 32);
        reg_name_2 = tacc_target_register_name(top_reg_2, 32);
        if (is_sext) {
            tacc_cg_output(
                state, "\n\t movl %s, %s, $31", reg_name, reg_name_2);
            tacc_cg_output(state, "\n\t sar %s, $31", reg_name_2);
        } else {
            tacc_cg_output(state, "\n\t xorl %s, %s", reg_name_2, reg_name_2);
        }
        return;
    }

    if (is_sext) {
        op_base = "movs";
    } else {
        op_base = "movz";
    }
    op_suff_from = tacc_target_op_suffix(from_width);
    op_suff_to = tacc_target_op_suffix(to_width);
    top_reg = tacc_cg_ensure_top_is_single(state, REG_ANY);
    reg_name = tacc_target_register_name(top_reg, from_width);
    reg_name_2 = tacc_target_register_name(top_reg, to_width);
    tacc_cg_output(state,
                   "\n\t %s%s%s %s, %s",
                   op_base,
                   op_suff_from,
                   op_suff_to,
                   reg_name,
                   reg_name_2);
}

void tacc_target_cg_xchg_reg_reg(struct tacc_cg_state *state,
                                 uint32_t reg_a,
                                 uint32_t reg_b) {
    char *a_name;
    char *b_name;
    a_name = tacc_target_register_as_32(reg_a);
    b_name = tacc_target_register_as_32(reg_b);
    tacc_cg_output(state, "\n\t xchgl %s, %s", a_name, b_name);
}

void tacc_target_cg_finalize(struct tacc_cg_state *state) {
    /* sp at 16k - 4 */
    tacc_cg_output_prelude(state, "\n\t pushl %%ebp");
    tacc_cg_output_prelude(state, "\n\t movl %%esp, %%ebp");

    /* sp at 16k - 8 */
    tacc_cg_output_prelude(state, "\n\t pushl %%ebx");
    /* sp at 16k - 12 */
    tacc_cg_output_prelude(state, "\n\t pushl %%esi");
    /* sp at 16k - 16 */
    tacc_cg_output_prelude(state, "\n\t pushl %%edi");

    /* no need to move params to stack, they should already all be on stack */

    tacc_cg_output_prelude(state,
                           "\n\t subl $%d, %%esp",
                           (int) (state->num_local_bytes + 0xF) & ~0xF);

    tacc_cg_output(state, "\n\t leal -12(%%ebp), %%esp");
    tacc_cg_output(state, "\n\t popl %%edi");
    tacc_cg_output(state, "\n\t popl %%esi");
    tacc_cg_output(state, "\n\t popl %%ebx");
    tacc_cg_output(state, "\n\t popl %%ebp");
    tacc_cg_output(state, "\n\t ret");
}

void tacc_target_cg_deref_int(struct tacc_cg_state *state,
                              struct tacc_type *int_type) {
    struct tacc_target_place_register *reg_place;
    struct tacc_target_place_register *reg_place_2;
    size_t load_width;
    struct tacc_slot *slot;
    uint32_t reg;
    uint32_t reg_2;

    load_width = tacc_type_bit_width(int_type);
    reg = tacc_cg_ensure_top_is_single(state, REG_ANY);
    if (load_width > 32) {
        reg_2 = tacc_target_cg_alloc_reg(state, REG_ANY & ~reg);
    }
    slot = tacc_cg_get_top(state);

    switch (load_width) {
    case 8:
    case 16:
    case 32:
        tacc_cg_output(state,
                       "\n\t mov%s (%s), %s",
                       tacc_target_op_suffix(load_width),
                       tacc_target_register_as_32(reg),
                       tacc_target_register_name(reg, load_width));
        slot->ty = int_type;
        break;
    case 64:
        tacc_cg_output(state,
                       "\n\t movl 4(%s), %s",
                       tacc_target_register_as_32(reg),
                       tacc_target_register_as_32(reg_2));
        tacc_cg_output(state,
                       "\n\t movl (%s), %s",
                       tacc_target_register_as_32(reg),
                       tacc_target_register_as_32(reg));
        reg_place = slot->place.reg;
        reg_place_2 = tacc_target_place_register_new();
        reg_place_2->reg = reg_2;
        slot->place_kind = PLACE_REGISTER_PAIR;
        slot->place.pair.reg = reg_place;
        slot->place.pair.reg_2 = reg_place_2;
        slot->ty = int_type;
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid load width %d", load_width);
    }
}

void tacc_target_cg_store_int(struct tacc_cg_state *state,
                              struct tacc_type *int_type) {
    size_t store_width;
    uint32_t reg;
    uint32_t reg_2;
    uint32_t addr_reg;

    store_width = tacc_type_bit_width(int_type);
    reg = tacc_cg_ensure_top_is_single(state, REG_ANY);
    if (store_width > 32) {
        tacc_cg_ensure_top_is_pair(state, &reg, &reg_2);
    } else {
        reg = tacc_cg_ensure_top_is_single(state, REG_ANY);
    }
    addr_reg = tacc_cg_ensure_over_is_single(state, REG_ANY);

    switch (store_width) {
    case 8:
    case 16:
    case 32:
        tacc_cg_output(state,
                       "\n\t mov%s %s, (%s)",
                       tacc_target_op_suffix(store_width),
                       tacc_target_register_name(reg, store_width),
                       tacc_target_register_as_32(addr_reg));
        break;
    case 64:
        tacc_cg_output(state,
                       "\n\t movl %s, 4(%s)",
                       tacc_target_register_as_32(reg_2),
                       tacc_target_register_as_32(addr_reg));
        tacc_cg_output(state,
                       "\n\t movl %s, (%s)",
                       tacc_target_register_as_32(reg),
                       tacc_target_register_as_32(addr_reg));
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "invalid store width %d", store_width);
    }
    tacc_cg_pop(state);
    tacc_cg_pop(state);
}

void tacc_target_cg_addrof_var(struct tacc_cg_state *state,
                               struct tacc_local_var *var) {
    struct tacc_target_place_register *reg_place;
    uint32_t reg;
    char *reg_name;

    reg = tacc_target_cg_alloc_reg(state, REG_ANY);
    reg_place = tacc_target_place_register_new();
    reg_place->reg = reg;

    reg_name = tacc_target_register_as_32(reg);
    tacc_cg_output(state, "\n\t leal %d(%%ebp), %s", var->offset, reg_name);
    tacc_cg_push_reg(
        state,
        reg_place,
        tacc_type_to_pointer(state->compiler->target->pointer_ty, var->ty, 1));
}

static tacc_bool tacc_target_type_fits_in_single_reg(struct tacc_type *ty) {
    if (ty->kind == TYK_PTR) {
        return 1;
    }
    if (tacc_type_is_integral(ty)) {
        if (tacc_type_bit_width(ty) <= 32) {
            return 1;
        }
    }
    return 0;
}

void tacc_target_cg_dup(struct tacc_cg_state *state) {
    struct tacc_slot *slot;
    struct tacc_target_place_register *reg_place;
    struct tacc_target_place_register *reg_place_2;
    uint32_t reg;
    uint32_t reg_2;
    uint32_t new_reg;
    uint32_t new_reg_2;

    slot = tacc_cg_get_top(state);
    if (tacc_target_type_fits_in_single_reg(slot->ty)) {
        reg = tacc_cg_ensure_top_is_single(state, REG_ANY);
        new_reg = tacc_target_cg_alloc_reg(state, REG_ANY & ~reg);
        tacc_target_cg_move_reg_reg(state, reg, new_reg);
        reg_place = tacc_target_place_register_new();
        reg_place->reg = new_reg;
        tacc_cg_push_reg(state, reg_place, slot->ty);
        return;
    }

    tacc_cg_ensure_top_is_pair(state, &reg, &reg_2);
    new_reg = tacc_target_cg_alloc_reg(state, REG_ANY & ~reg);
    new_reg_2 = tacc_target_cg_alloc_reg(state, REG_ANY & ~(reg | new_reg));
    tacc_target_cg_move_reg_reg(state, reg, new_reg);
    tacc_target_cg_move_reg_reg(state, reg_2, new_reg_2);
    reg_place = tacc_target_place_register_new();
    reg_place->reg = new_reg;
    reg_place_2 = tacc_target_place_register_new();
    reg_place_2->reg = new_reg_2;
    tacc_cg_push_reg_pair(state, reg_place, reg_place_2, slot->ty);
}

void tacc_target_cg_addrof_obj(struct tacc_cg_state *state,
                               struct tacc_global_object *object) {
    uint32_t reg;
    struct tacc_target_place_register *reg_place;
    char *reg_name;
    struct tacc_string *symbol_name;

    reg = tacc_target_cg_alloc_reg(state, REG_ANY);
    reg_name = tacc_target_register_as_32(reg);
    symbol_name = tacc_compile_get_name(state->compiler, object->name_ref);
    tacc_cg_output(state,
                   "\n\t movl $%s, %s",
                   tacc_dynstring_as_str(symbol_name),
                   reg_name);
    reg_place = tacc_target_place_register_new();
    reg_place->reg = reg;
    tacc_cg_push_reg(state,
                     reg_place,
                     tacc_type_to_pointer(state->compiler->target->pointer_ty,
                                          object->extra.obj_type,
                                          1));
}

void tacc_target_cg_alloc_stack(struct tacc_cg_state *state,
                                size_t space,
                                size_t align_p2) {
    if (align_p2 > 3) {
        tacc_cg_output(state, "\n\t andl $-%d, %%esp", 1 << align_p2);
    }
#ifdef __M2__
    tacc_cg_output(state, "\n\t subl $%d, %%esp", space);
#else
    tacc_cg_output(state, "\n\t subl $%" PRIsz ", %%esp", space);
#endif
}

void tacc_target_cg_load_scratch_part(struct tacc_cg_state *state,
                                      int offset,
                                      uint32_t to_reg,
                                      struct tacc_type *ty) {
    size_t width;

    width = tacc_type_bit_width(ty);
    tacc_cg_output(state,
                   "\n\t mov%s %d(%%ebp), %s",
                   tacc_target_op_suffix(width),
                   offset,
                   tacc_target_register_name(to_reg, width));
}

void tacc_target_cg_move_scratch_to_stack(struct tacc_cg_state *state,
                                          int from_fp_offset,
                                          int to_sp_offset,
                                          size_t size) {
    size_t i;

    tacc_assert(ASSERT_ICE,
                (size & 3) == 0,
                "move_scratch_to_stack: not a multiple of 4");
    for (i = 0; i < size; i = i + 4) {
        tacc_cg_output(
            state, "\n\t movl %d(%%ebp), %%eax", from_fp_offset + (int) size);
        tacc_cg_output(
            state, "\n\t movl %%eax, %d(%%esp)", to_sp_offset + (int) size);
    }
}

void tacc_target_cg_store_reg_to_scratch(struct tacc_cg_state *state,
                                         int offset,
                                         uint32_t reg,
                                         struct tacc_type *ty) {
    tacc_cg_output(state,
                   "\n\t mov%s %s, %d(%%ebp)",
                   tacc_target_op_suffix(tacc_type_bit_width(ty)),
                   tacc_target_register_name(reg, tacc_type_bit_width(ty)),
                   offset);
}

void tacc_target_cg_store_reg_pair_to_scratch(struct tacc_cg_state *state,
                                              int offset,
                                              uint32_t reg,
                                              uint32_t reg_2) {
    tacc_cg_output(state,
                   "\n\t movl %s, %d(%%ebp)",
                   tacc_target_register_as_32(reg),
                   offset);
    tacc_cg_output(state,
                   "\n\t movl %s, %d(%%ebp)",
                   tacc_target_register_as_32(reg_2),
                   offset + 4);
}

void tacc_target_cg_call_top(struct tacc_cg_state *state) {
    struct tacc_slot *slot;

    slot = tacc_cg_get_top(state);

    tacc_assert(ASSERT_ICE,
                slot->place_kind == PLACE_SCRATCH,
                "expected callee pointer to be in scratch");
    tacc_cg_output(state, "\n\t movl %d(%%ebp), %%eax", slot->place.offset);
    tacc_cg_pop(state);
    tacc_cg_output(state, "\n\t call %%eax");
}

void tacc_target_cg_normalize_retval(struct tacc_cg_state *state,
                                     struct tacc_callitf *itf,
                                     struct tacc_type *return_ty) {
    struct tacc_target_place_register *reg_place;
    struct tacc_target_place_register *reg_place_2;

    switch (itf->retval_kind) {
    case CALLITF_RETVAL_NONE:
        tacc_cg_push_void(state);
        break;
    case CALLITF_RETVAL_REGISTER:
        tacc_assert(ASSERT_TODO,
                    itf->retval_reg_class <= REGC_INT_D,
                    "returning non-integer register");
        reg_place = tacc_target_place_register_new();
        reg_place->reg = itf->retval_reg;
        tacc_cg_push_reg(state, reg_place, return_ty);
        break;
    case CALLITF_RETVAL_REGISTER_PAIR:
        tacc_assert(ASSERT_ICE,
                    itf->retval_reg_class == REGC_INT_D,
                    "returning non-integer register pair");
        reg_place = tacc_target_place_register_new();
        reg_place->reg = itf->retval_reg;
        reg_place_2 = tacc_target_place_register_new();
        reg_place_2->reg = itf->retval_reg_2;
        tacc_cg_push_reg_pair(state, reg_place, reg_place_2, return_ty);
        break;
    case CALLITF_RETVAL_OUTPARAM:
        tacc_assert(ASSERT_TODO, 0, "outparam returns");
        break;
    }
}

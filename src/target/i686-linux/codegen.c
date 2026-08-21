#include "codegen.h"
#include "compile.h"
#include "machine.h"
#include "target/i686-linux/registers.h"
#include "target/target.h"
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
        tacc_assert(0, "invalid width %d", width);
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
        tacc_assert(0, "invalid width %d", width);
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

void tacc_target_cg_int(struct tacc_cg_state *state,
                        struct tacc_val *val,
                        size_t width) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    char *reg;

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

void tacc_target_cg_return_top_int(struct tacc_cg_state *state, size_t width) {
    if (width > 32) {
        tacc_cg_move_pair(state, tacc_cg_get_top(state), REG_EAX, REG_EDX);
    } else {
        tacc_cg_move(state, tacc_cg_get_top(state), REG_EAX);
    }
    tacc_cg_pop(state);
    tacc_cg_output(state, "\n\t jmp .Lepilog");
}

void tacc_target_cg_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

void tacc_target_cg_state_free(struct tacc_target_cg_state *state) {
    tacc_free(state);
}

void tacc_target_cg_ext_top(struct tacc_cg_state *state,
                            size_t from_width,
                            size_t to_width,
                            tacc_bool is_sext) {
    struct tacc_slot *slot;
    struct tacc_target_place_register *top_place;
    enum tacc_target_register top_reg;
    enum tacc_target_register top_reg_2;
    char *reg_name;
    char *reg_name_2;
    char *op_base;
    char *op_suff_from;
    char *op_suff_to;

    if (from_width <= 32 && to_width > 32) {
        tacc_target_cg_ext_top(state, from_width, 32, is_sext);
        slot = tacc_cg_get_top(state);
        tacc_assert(slot->place_kind == PLACE_REGISTER,
                    "top must be single when extending with from_width <= 32");
        top_place = slot->place.reg;
        top_reg_2 = tacc_target_cg_alloc_reg(state, REG_ANY);
        slot->place_kind = PLACE_REGISTER_PAIR;
        slot->place.pair.reg = top_place;
        slot->place.pair.reg_2 = tacc_target_place_register_new();
        slot->place.pair.reg_2->reg = top_reg_2;
        reg_name = tacc_target_register_name(top_place->reg, 32);
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
    top_reg = tacc_cg_ensure_top_is_single(state);
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
    tacc_cg_output(state, "\n\t xchgq %s, %s", a_name, b_name);
}

void tacc_target_cg_finalize(struct tacc_cg_state *state) {
    /* sp at 16k - 4 */
    tacc_cg_output_prelude(state, "\n\t pushl %%ebp");
    /* sp at 16k - 8 */
    tacc_cg_output_prelude(state, "\n\t subl $8, %%esp");
    /* sp at 16k */
    tacc_cg_output_prelude(state, "\n\t movl %%esp, %%ebp");
    tacc_cg_output_prelude(state,
                           "\n\t subl $%d, %%esp",
                           (int) (state->num_local_bytes + 0xF) & ~0xF);

    tacc_cg_output(state, "\n\t movl %%ebp, %%esp");
    tacc_cg_output(state, "\n\t addl $8, %%esp");
    tacc_cg_output(state, "\n\t popl %%ebp");
    tacc_cg_output(state, "\n\t ret");
}

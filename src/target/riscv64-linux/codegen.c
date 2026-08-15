#include "codegen.h"
#include "compile.h"
#include "util.h"

enum tacc_target_register {
    REG_T0 = 0x1,
    REG_T1 = 0x2,
    REG_S1 = 0x4,
    REG_A0 = 0x8,
    REG_A1 = 0x10,
    REG_A2 = 0x20,
    REG_A3 = 0x40,
    REG_A4 = 0x80,
    REG_A5 = 0x100,
    REG_A6 = 0x200,
    REG_A7 = 0x400,
    REG_S2 = 0x800,
    REG_S3 = 0x1000,
    REG_S4 = 0x2000,
    REG_S5 = 0x4000,
    REG_S6 = 0x8000,
    REG_S7 = 0x10000,
    REG_S8 = 0x20000,
    REG_S9 = 0x40000,
    REG_S10 = 0x80000,
    REG_S11 = 0x100000,
    REG_T3 = 0x200000,
    REG_T4 = 0x400000,
    REG_T5 = 0x800000,
    REG_T6 = 0x1000000,
};

#define REG_ANY 0x1FFFFFF

struct tacc_target_codegen_state {
    int x;
};

struct tacc_target_place_register {
    enum tacc_target_register reg;
};

struct tacc_target_codegen_state *tacc_target_codegen_state_new(void) {
    struct tacc_target_codegen_state *state;

    state = tacc_malloc(sizeof(struct tacc_target_codegen_state));

    return state;
}

static enum tacc_target_register tacc_target_codegen_alloc_reg(
    struct tacc_codegen_state *state, uint32_t desired_registers) {
    size_t i;
    size_t oldest_matching;
    tacc_bool found_matching;
    uint32_t occupied_registers;
    uint32_t available;
    struct tacc_slot_list_entry *slot_entry;
    enum tacc_target_register reg_chosen;

    /* steal slot from oldest stack entry that uses a desirable register */
    found_matching = 0;
    occupied_registers = 0;
    oldest_matching = 0;
    for (i = 0; i < tacc_slot_list_len(state->stack); i = i + 1) {
        slot_entry = tacc_slot_list_get(state->stack, i);
        if (slot_entry->content->place_kind == PLACE_REGISTER) {
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
        tacc_codegen_slot_spill(state, slot_entry->content);
        return reg_chosen;
    }
    available = desired_registers & ~(occupied_registers);
    return available & (-available);
}

static char *tacc_target_register_as_64(enum tacc_target_register reg) {
    switch (reg) {
    case REG_T0:
        return "t0";
    case REG_T1:
        return "t1";
    case REG_S1:
        return "s1";
    case REG_A0:
        return "a0";
    case REG_A1:
        return "a1";
    case REG_A2:
        return "a2";
    case REG_A3:
        return "a3";
    case REG_A4:
        return "a4";
    case REG_A5:
        return "a5";
    case REG_A6:
        return "a6";
    case REG_A7:
        return "a7";
    case REG_S2:
        return "s2";
    case REG_S3:
        return "s3";
    case REG_S4:
        return "s4";
    case REG_S5:
        return "s5";
    case REG_S6:
        return "s6";
    case REG_S7:
        return "s7";
    case REG_S8:
        return "s8";
    case REG_S9:
        return "s9";
    case REG_S10:
        return "s10";
    case REG_S11:
        return "s11";
    case REG_T3:
        return "t3";
    case REG_T4:
        return "t4";
    case REG_T5:
        return "t5";
    case REG_T6:
        return "t6";
    default:
        return "INVALID-64";
    }
}

static struct tacc_target_place_register *tacc_target_place_register_new(void) {
    struct tacc_target_place_register *reg;

    reg = tacc_malloc(sizeof(struct tacc_target_place_register));
    reg->reg = 0;

    return reg;
}

void tacc_target_codegen_int(struct tacc_codegen_state *state,
                             struct tacc_val *val) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    char *reg;

    register_place = tacc_target_codegen_alloc_reg(state, REG_ANY);
    reg = tacc_target_register_as_64(register_place);
    tacc_codegen_output(
        state, "\n\t li %s, 0x%x", reg, val->value.int_value->high);
    tacc_codegen_output(state, "\n\t slli %s, %s, 12", reg, reg);
    tacc_codegen_output(state,
                        "\n\t addi %s, %s, 0x%x",
                        reg,
                        reg,
                        (val->value.int_value->low >> 20) & 0x3FF);
    tacc_codegen_output(state, "\n\t slli %s, %s, 12", reg, reg);
    tacc_codegen_output(state,
                        "\n\t addi %s, %s, 0x%x",
                        reg,
                        reg,
                        (val->value.int_value->low >> 8) & 0x3FF);
    tacc_codegen_output(state, "\n\t slli %s, %s, 8", reg, reg);
    tacc_codegen_output(state,
                        "\n\t addi %s, %s, 0x%x",
                        reg,
                        reg,
                        val->value.int_value->low & 0xFF);

    reg_place = tacc_target_place_register_new();
    reg_place->reg = register_place;
    tacc_codegen_push_reg(state, reg_place, val->type);
}

static void tacc_target_codegen_move(struct tacc_codegen_state *state,
                                     struct tacc_slot *slot,
                                     uint32_t to_reg) {
    enum tacc_target_register new_reg;

    if (slot->place_kind == PLACE_REGISTER &&
        (slot->place.reg->reg & to_reg) != 0) {
        return;
    }
    new_reg = tacc_target_codegen_alloc_reg(state, to_reg);
    if (slot->place_kind == PLACE_REGISTER) {
        tacc_codegen_output(state,
                            "\n\t mv %s, %s",
                            tacc_target_register_as_64(new_reg),
                            tacc_target_register_as_64(slot->place.reg->reg));
        slot->place.reg->reg = new_reg;
    } else {
        tacc_assert(0, "TODO: move from stack to register");
    }
}

void tacc_target_codegen_return_top_int(struct tacc_codegen_state *state) {
    tacc_target_codegen_move(state, tacc_codegen_get_top(state), REG_A0);
    tacc_codegen_pop(state);
    tacc_codegen_output(state, "\n\t j .Lepilog");
}

void tacc_target_codegen_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

#include "codegen.h"
#include "compile.h"
#include "util.h"

enum tacc_target_register {
    REG_X0 = 0x1,
    REG_X1 = 0x2,
    REG_X2 = 0x4,
    REG_X3 = 0x8,
    REG_X4 = 0x10,
    REG_X5 = 0x20,
    REG_X6 = 0x40,
    REG_X7 = 0x80,
    REG_X8 = 0x100,
    REG_X9 = 0x200,
    REG_X10 = 0x400,
    REG_X11 = 0x800,
    REG_X12 = 0x1000,
    REG_X13 = 0x2000,
    REG_X14 = 0x4000,
    REG_X15 = 0x8000,
    REG_X19 = 0x10000,
    REG_X20 = 0x20000,
    REG_X21 = 0x40000,
    REG_X22 = 0x80000,
    REG_X23 = 0x100000,
    REG_X24 = 0x200000,
    REG_X25 = 0x400000,
    REG_X26 = 0x800000,
    REG_X27 = 0x1000000,
    REG_X28 = 0x2000000,
};

#define REG_ANY 0x3FFFFFF

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
    case REG_X0:
        return "x0";
    case REG_X1:
        return "x1";
    case REG_X2:
        return "x2";
    case REG_X3:
        return "x3";
    case REG_X4:
        return "x4";
    case REG_X5:
        return "x5";
    case REG_X6:
        return "x6";
    case REG_X7:
        return "x7";
    case REG_X8:
        return "x8";
    case REG_X9:
        return "x9";
    case REG_X10:
        return "x10";
    case REG_X11:
        return "x11";
    case REG_X12:
        return "x12";
    case REG_X13:
        return "x13";
    case REG_X14:
        return "x14";
    case REG_X15:
        return "x15";
    case REG_X19:
        return "x19";
    case REG_X20:
        return "x20";
    case REG_X21:
        return "x21";
    case REG_X22:
        return "x22";
    case REG_X23:
        return "x23";
    case REG_X24:
        return "x24";
    case REG_X25:
        return "x25";
    case REG_X26:
        return "x26";
    case REG_X27:
        return "x27";
    case REG_X28:
        return "x28";
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
        state, "\n\t movz %s, #0x%x", reg, val->value.int_value->low & 0xFFFF);
    if ((val->value.int_value->low >> 16) != 0) {
        tacc_codegen_output(state,
                            "\n\t movk %s, #0x%x, lsl #16",
                            reg,
                            (val->value.int_value->low >> 16) & 0xFFFF);
    }
    if ((val->value.int_value->high & 0xFFFF) != 0) {
        tacc_codegen_output(state,
                            "\n\t movk %s, #0x%x, lsl #32",
                            reg,
                            (val->value.int_value->high) & 0xFFFF);
    }
    if ((val->value.int_value->high >> 16) != 0) {
        tacc_codegen_output(state,
                            "\n\t movk %s, #0x%x, lsl #48",
                            reg,
                            (val->value.int_value->high >> 16) & 0xFFFF);
    }

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
                            "\n\t mov %s, %s",
                            tacc_target_register_as_64(slot->place.reg->reg),
                            tacc_target_register_as_64(new_reg));
        slot->place.reg->reg = new_reg;
    } else {
        tacc_assert(0, "TODO: move from stack to register");
    }
}

void tacc_target_codegen_return_top_int(struct tacc_codegen_state *state) {
    tacc_target_codegen_move(state, tacc_codegen_get_top(state), REG_X0);
    tacc_codegen_pop(state);
    tacc_codegen_output(state, "\n\t b .Lepilog");
}

void tacc_target_codegen_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

#include "codegen.h"
#include "compile.h"
#include "machine.h"
#include "target/aarch64-linux/registers.h"
#include "util.h"

struct tacc_target_cg_state {
    int x;
};

struct tacc_target_cg_state *tacc_target_cg_state_new(void) {
    struct tacc_target_cg_state *state;

    state = tacc_malloc(sizeof(struct tacc_target_cg_state));

    return state;
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

static char *tacc_target_register_as_32(enum tacc_target_register reg) {
    switch (reg) {
    case REG_X0:
        return "w0";
    case REG_X1:
        return "w1";
    case REG_X2:
        return "w2";
    case REG_X3:
        return "w3";
    case REG_X4:
        return "w4";
    case REG_X5:
        return "w5";
    case REG_X6:
        return "w6";
    case REG_X7:
        return "w7";
    case REG_X8:
        return "w8";
    case REG_X9:
        return "w9";
    case REG_X10:
        return "w10";
    case REG_X11:
        return "w11";
    case REG_X12:
        return "w12";
    case REG_X13:
        return "w13";
    case REG_X14:
        return "w14";
    case REG_X15:
        return "w15";
    case REG_X19:
        return "w19";
    case REG_X20:
        return "w20";
    case REG_X21:
        return "w21";
    case REG_X22:
        return "w22";
    case REG_X23:
        return "w23";
    case REG_X24:
        return "w24";
    case REG_X25:
        return "w25";
    case REG_X26:
        return "w26";
    case REG_X27:
        return "w27";
    case REG_X28:
        return "w28";
    default:
        return "INVALID-32";
    }
}

void tacc_target_cg_int(struct tacc_cg_state *state,
                        struct tacc_val *val,
                        size_t width) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    char *reg;

    register_place = tacc_target_cg_alloc_reg(state, REG_ANY);
    if (width <= 32) {
        reg = tacc_target_register_as_32(register_place);
    } else {
        reg = tacc_target_register_as_64(register_place);
    }
    tacc_cg_output(
        state, "\n\t movz %s, #0x%x", reg, val->value.int_value->low & 0xFFFF);
    if ((val->value.int_value->low >> 16) != 0) {
        tacc_cg_output(state,
                       "\n\t movk %s, #0x%x, lsl #16",
                       reg,
                       (val->value.int_value->low >> 16) & 0xFFFF);
    }
    if (width > 32) {
        if ((val->value.int_value->high & 0xFFFF) != 0) {
            tacc_cg_output(state,
                           "\n\t movk %s, #0x%x, lsl #32",
                           reg,
                           (val->value.int_value->high) & 0xFFFF);
        }
        if ((val->value.int_value->high >> 16) != 0) {
            tacc_cg_output(state,
                           "\n\t movk %s, #0x%x, lsl #48",
                           reg,
                           (val->value.int_value->high >> 16) & 0xFFFF);
        }
    }

    reg_place = tacc_target_place_register_new();
    reg_place->reg = register_place;
    tacc_cg_push_reg(state, reg_place, val->type);
}

static void tacc_target_cg_move(struct tacc_cg_state *state,
                                struct tacc_slot *slot,
                                uint32_t to_reg) {
    enum tacc_target_register new_reg;

    if (slot->place_kind == PLACE_REGISTER &&
        (slot->place.reg->reg & to_reg) != 0) {
        return;
    }
    new_reg = tacc_target_cg_alloc_reg(state, to_reg);
    if (slot->place_kind == PLACE_REGISTER) {
        tacc_cg_output(state,
                       "\n\t mov %s, %s",
                       tacc_target_register_as_64(slot->place.reg->reg),
                       tacc_target_register_as_64(new_reg));
        slot->place.reg->reg = new_reg;
    } else {
        tacc_assert(0, "TODO: move from stack to register");
    }
}

void tacc_target_cg_return_top_int(struct tacc_cg_state *state) {
    tacc_target_cg_move(state, tacc_cg_get_top(state), REG_X0);
    tacc_cg_pop(state);
    tacc_cg_output(state, "\n\t b .Lepilog");
}

void tacc_target_cg_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

void tacc_target_cg_state_free(struct tacc_target_cg_state *state) {
    tacc_free(state);
}

tacc_bool tacc_type_needs_reg_pair(struct tacc_type *ty) {
    TACC_UNUSED(ty);

    return 0;
}

void tacc_target_cg_ext_top(struct tacc_cg_state *state,
                            size_t from_width,
                            size_t to_width,
                            tacc_bool is_sext) {
    struct tacc_slot *slot;
    enum tacc_target_register top_reg;
    char *reg_name;
    int width;

    slot = tacc_cg_get_top(state);
    tacc_target_cg_move(state, slot, REG_ANY);
    top_reg = slot->place.reg->reg;
    width = (int) from_width;
    if (to_width <= 32) {
        reg_name = tacc_target_register_as_32(top_reg);
    } else {
        reg_name = tacc_target_register_as_64(top_reg);
    }
    if (is_sext) {
        tacc_cg_output(
            state, "\n\t sbfx %s, %s, #0, #%d", reg_name, reg_name, width);
    } else {
        tacc_cg_output(
            state, "\n\t ubfx %s, %s, #0, #%d", reg_name, reg_name, width);
    }
}

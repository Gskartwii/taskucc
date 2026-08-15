#include "codegen.h"
#include "compile.h"
#include "target/riscv64-linux/registers.h"
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

void tacc_target_cg_int(struct tacc_cg_state *state, struct tacc_val *val) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    char *reg;

    register_place = tacc_target_cg_alloc_reg(state, REG_ANY);
    reg = tacc_target_register_as_64(register_place);
    tacc_cg_output(state, "\n\t li %s, 0x%x", reg, val->value.int_value->high);
    tacc_cg_output(state, "\n\t slli %s, %s, 12", reg, reg);
    tacc_cg_output(state,
                   "\n\t addi %s, %s, 0x%x",
                   reg,
                   reg,
                   (val->value.int_value->low >> 20) & 0x3FF);
    tacc_cg_output(state, "\n\t slli %s, %s, 12", reg, reg);
    tacc_cg_output(state,
                   "\n\t addi %s, %s, 0x%x",
                   reg,
                   reg,
                   (val->value.int_value->low >> 8) & 0x3FF);
    tacc_cg_output(state, "\n\t slli %s, %s, 8", reg, reg);
    tacc_cg_output(state,
                   "\n\t addi %s, %s, 0x%x",
                   reg,
                   reg,
                   val->value.int_value->low & 0xFF);

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
                       "\n\t mv %s, %s",
                       tacc_target_register_as_64(new_reg),
                       tacc_target_register_as_64(slot->place.reg->reg));
        slot->place.reg->reg = new_reg;
    } else {
        tacc_assert(0, "TODO: move from stack to register");
    }
}

void tacc_target_cg_return_top_int(struct tacc_cg_state *state) {
    tacc_target_cg_move(state, tacc_cg_get_top(state), REG_A0);
    tacc_cg_pop(state);
    tacc_cg_output(state, "\n\t j .Lepilog");
}

void tacc_target_cg_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

void tacc_target_cg_state_free(struct tacc_target_cg_state *state) {
    tacc_free(state);
}

#include "codegen.h"
#include "compile.h"
#include "target/x86_64-linux/registers.h"
#include "util.h"

struct tacc_target_codegen_state {
    int x;
};

struct tacc_target_codegen_state *tacc_target_codegen_state_new(void) {
    struct tacc_target_codegen_state *state;

    state = tacc_malloc(sizeof(struct tacc_target_codegen_state));

    return state;
}

static char *tacc_target_register_as_64(enum tacc_target_register reg) {
    switch (reg) {
    case REG_RAX:
        return "%rax";
    case REG_RBX:
        return "%rbx";
    case REG_RCX:
        return "%rcx";
    case REG_RDX:
        return "%rdx";
    case REG_RSI:
        return "%rsi";
    case REG_RDI:
        return "%rdi";
    case REG_R8:
        return "%r8";
    case REG_R9:
        return "%r9";
    case REG_R10:
        return "%r10";
    case REG_R11:
        return "%r11";
    case REG_R12:
        return "%r12";
    case REG_R13:
        return "%r13";
    case REG_R14:
        return "%r14";
    case REG_R15:
        return "%r15";
    default:
        return "INVALID-64";
    }
}

void tacc_target_codegen_int(struct tacc_codegen_state *state,
                             struct tacc_val *val) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    char *reg;

    register_place = tacc_target_codegen_alloc_reg(state, REG_ANY);
    reg = tacc_target_register_as_64(register_place);
    if (val->value.int_value->high == 0) {
        if ((val->value.int_value->low & 0x80000000) != 0) {
            tacc_codegen_output(
                state, "\n\t movabs $0x%x, %s", val->value.int_value->low, reg);
        } else {
            tacc_codegen_output(
                state, "\n\t movq $0x%x, %s", val->value.int_value->low, reg);
        }
    } else if (IS_U32_MAX(val->value.int_value->high)) {
        if ((val->value.int_value->low & 0x80000000) != 0) {
            tacc_codegen_output(state,
                                "\n\t movq $-0x%x, %s",
                                -(val->value.int_value->low),
                                reg);
        } else {
            tacc_codegen_output(state, "\n\t movq $-1, %s", reg);
            tacc_codegen_output(state, "\n\t shlq $32, %s", reg);
            tacc_codegen_output(
                state, "\n\t orq $0x%x, %s", val->value.int_value->low, reg);
        }
    } else {
        if ((val->value.int_value->low & 0x80000000) == 0) {
            tacc_codegen_output(state,
                                "\n\t movabs $0x%x, %s",
                                val->value.int_value->high,
                                reg);
            tacc_codegen_output(state, "\n\t shlq $32, %s", reg);
            tacc_codegen_output(
                state, "\n\t addq $0x%x, %s", val->value.int_value->low, reg);
        } else {
            tacc_codegen_output(state,
                                "\n\t movabs $0x%x, %s",
                                val->value.int_value->high + 1,
                                reg);
            tacc_codegen_output(state, "\n\t shlq $32, %s", reg);
            tacc_codegen_output(state,
                                "\n\t subq $0x%x, %s",
                                -(val->value.int_value->low),
                                reg);
        }
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
                            "\n\t movq %s, %s",
                            tacc_target_register_as_64(slot->place.reg->reg),
                            tacc_target_register_as_64(new_reg));
        slot->place.reg->reg = new_reg;
    } else {
        tacc_assert(0, "TODO: move from stack to register");
    }
}

void tacc_target_codegen_return_top_int(struct tacc_codegen_state *state) {
    tacc_target_codegen_move(state, tacc_codegen_get_top(state), REG_RAX);
    tacc_codegen_pop(state);
    tacc_codegen_output(state, "\n\t jmp .Lepilog");
}

void tacc_target_codegen_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

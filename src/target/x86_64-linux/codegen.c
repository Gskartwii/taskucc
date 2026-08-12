#include "codegen.h"
#include "compile.h"
#include "util.h"

enum tacc_target_register {
    REG_RAX = 0x1,
    REG_RBX = 0x2,
    REG_RCX = 0x4,
    REG_RDX = 0x8,
    REG_RSI = 0x10,
    REG_RDI = 0x20,
    REG_R8 = 0x40,
    REG_R9 = 0x80,
    REG_R10 = 0x100,
    REG_R11 = 0x200,
    REG_R12 = 0x400,
    REG_R13 = 0x800,
    REG_R14 = 0x1000,
    REG_R15 = 0x2000,
};

#define REG_ANY 0x3FFF

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

#include "codegen.h"
#include "compile.h"
#include "type.h"
#include "util.h"

enum tacc_target_register {
    REG_EAX = 0x1,
    REG_EBX = 0x2,
    REG_ECX = 0x4,
    REG_EDX = 0x8,
    REG_ESI = 0x10,
    REG_EDI = 0x20,
};

#define REG_ANY 0x3F

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
    tacc_assert(val->type->kind != TYK_SLONGLONG &&
                    val->type->kind != TYK_ULONGLONG,
                "TODO: long-long on i686");
    reg = tacc_target_register_as_32(register_place);
    tacc_codegen_output(
        state, "\n\t movl $0x%x, %s", val->value.int_value->low, reg);

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
                            tacc_target_register_as_32(slot->place.reg->reg),
                            tacc_target_register_as_32(new_reg));
        slot->place.reg->reg = new_reg;
    } else {
        tacc_assert(0, "TODO: move from stack to register");
    }
}

void tacc_target_codegen_return_top_int(struct tacc_codegen_state *state) {
    tacc_target_codegen_move(state, tacc_codegen_get_top(state), REG_EAX);
    tacc_codegen_pop(state);
    tacc_codegen_output(state, "\n\t jmp .Lepilog");
}

void tacc_target_codegen_prelude(struct tacc_compiler *compiler) {
    /* When using GNU linker, avoid warning about executable stack. */
    tacc_compile_output_directive(compiler,
                                  "section .note.GNU-stack,\"\",@progbits");
}

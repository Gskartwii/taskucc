#include "codegen.h"
#include "compile.h"
#include "target/i686-linux/registers.h"
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

static tacc_bool tacc_type_is_reg_pair(struct tacc_type *ty) {
    return ty->kind == TYK_SLONGLONG || ty->kind == TYK_ULONGLONG;
}

void tacc_target_cg_int(struct tacc_cg_state *state, struct tacc_val *val) {
    enum tacc_target_register register_place;
    enum tacc_target_register register_place_high;
    struct tacc_target_place_register *reg_place;
    char *reg;

    register_place = tacc_target_cg_alloc_reg(state, REG_ANY);
    register_place_high = 0;
    if (tacc_type_is_reg_pair(val->type)) {
        register_place_high =
            tacc_target_cg_alloc_reg(state, REG_ANY & ~register_place);
    }

    reg = tacc_target_register_as_32(register_place);
    tacc_cg_output(
        state, "\n\t movl $0x%x, %s", val->value.int_value->low, reg);
    if (register_place_high != 0) {
        reg = tacc_target_register_as_32(register_place_high);
        tacc_cg_output(
            state, "\n\t movl $0x%x, %s", val->value.int_value->high, reg);
    }

    reg_place = tacc_target_place_register_new();
    reg_place->reg = register_place | register_place_high;
    tacc_cg_push_reg(state, reg_place, val->type);
}

static void tacc_target_cg_move(struct tacc_cg_state *state,
                                struct tacc_slot *slot,
                                uint32_t to_reg,
                                uint32_t hi_reg) {
    enum tacc_target_register new_reg;

    if (slot->place_kind == PLACE_REGISTER &&
        (slot->place.reg->reg & to_reg) != 0) {
        return;
    }
    if (slot->place_kind == PLACE_REGISTER_PAIR &&
        slot->place.reg->reg == (to_reg | hi_reg)) {
        return;
    }
    if (slot->place_kind == PLACE_REGISTER) {
        new_reg = tacc_target_cg_alloc_reg(state, to_reg);
        tacc_cg_output(state,
                       "\n\t movl %s, %s",
                       tacc_target_register_as_32(slot->place.reg->reg),
                       tacc_target_register_as_32(new_reg));
        slot->place.reg->reg = new_reg;
    } else {
        tacc_assert(0, "TODO: move from stack to register");
    }
}

void tacc_target_cg_return_top_int(struct tacc_cg_state *state) {
    tacc_target_cg_move(state, tacc_cg_get_top(state), REG_EAX, REG_EDX);
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

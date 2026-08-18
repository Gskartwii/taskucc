#include "codegen.h"
#include "compile.h"
#include "machine.h"
#include "target/x86_64-linux/registers.h"
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

static char *tacc_target_register_as_32(enum tacc_target_register reg) {
    switch (reg) {
    case REG_RAX:
        return "%eax";
    case REG_RBX:
        return "%ebx";
    case REG_RCX:
        return "%ecx";
    case REG_RDX:
        return "%edx";
    case REG_RSI:
        return "%esi";
    case REG_RDI:
        return "%edi";
    case REG_R8:
        return "%r8d";
    case REG_R9:
        return "%r9d";
    case REG_R10:
        return "%r10d";
    case REG_R11:
        return "%r11d";
    case REG_R12:
        return "%r12d";
    case REG_R13:
        return "%r13d";
    case REG_R14:
        return "%r14d";
    case REG_R15:
        return "%r15d";
    default:
        return "INVALID-32";
    }
}

static char *tacc_target_register_as_16(enum tacc_target_register reg) {
    switch (reg) {
    case REG_RAX:
        return "%ax";
    case REG_RBX:
        return "%bx";
    case REG_RCX:
        return "%cx";
    case REG_RDX:
        return "%dx";
    case REG_RSI:
        return "%si";
    case REG_RDI:
        return "%di";
    case REG_R8:
        return "%r8w";
    case REG_R9:
        return "%r9w";
    case REG_R10:
        return "%r10w";
    case REG_R11:
        return "%r11w";
    case REG_R12:
        return "%r12w";
    case REG_R13:
        return "%r13w";
    case REG_R14:
        return "%r14w";
    case REG_R15:
        return "%r15w";
    default:
        return "INVALID-16";
    }
}

static char *tacc_target_register_as_8(enum tacc_target_register reg) {
    switch (reg) {
    case REG_RAX:
        return "%al";
    case REG_RBX:
        return "%bl";
    case REG_RCX:
        return "%cl";
    case REG_RDX:
        return "%dl";
    case REG_RSI:
        return "%sil";
    case REG_RDI:
        return "%dil";
    case REG_R8:
        return "%r8b";
    case REG_R9:
        return "%r9b";
    case REG_R10:
        return "%r10b";
    case REG_R11:
        return "%r11b";
    case REG_R12:
        return "%r12b";
    case REG_R13:
        return "%r13b";
    case REG_R14:
        return "%r14b";
    case REG_R15:
        return "%r15b";
    default:
        return "INVALID-16";
    }
}

static char *tacc_target_register_name(enum tacc_target_register reg,
                                       size_t width) {
    switch (width) {
    case 64:
        return tacc_target_register_as_64(reg);
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
    case 64:
        return "q";
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

void tacc_target_cg_int(struct tacc_cg_state *state,
                        struct tacc_val *val,
                        size_t width) {
    enum tacc_target_register register_place;
    struct tacc_target_place_register *reg_place;
    char *reg;
    char *op_suff;

    register_place = tacc_target_cg_alloc_reg(state, REG_ANY);
    reg = tacc_target_register_name(register_place, width);
    op_suff = tacc_target_op_suffix(width);
    if (val->value.int_value->high == 0 || width <= 32) {
        tacc_cg_output(state,
                       "\n\t mov%s $0x%x, %s",
                       op_suff,
                       val->value.int_value->low,
                       reg);
    } else {
        tacc_cg_output(
            state, "\n\t movq $0x%x, %s", val->value.int_value->high, reg);
        tacc_cg_output(state, "\n\t shlq $32, %s", reg);
        tacc_cg_output(
            state, "\n\t addq $0x%x, %s", val->value.int_value->low, reg);
    }

    reg_place = tacc_target_place_register_new();
    reg_place->reg = register_place;
    tacc_cg_push_reg(state, reg_place, val->type);
}

void tacc_target_cg_return_top_int(struct tacc_cg_state *state) {
    tacc_cg_move(state, tacc_cg_get_top(state), REG_RAX);
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

tacc_bool tacc_type_needs_reg_pair(struct tacc_type *ty) {
    TACC_UNUSED(ty);

    return 0;
}

void tacc_target_cg_ext_top(struct tacc_cg_state *state,
                            size_t from_width,
                            size_t to_width,
                            tacc_bool is_sext) {
    enum tacc_target_register top_reg;
    char *reg_name;
    char *reg_name_2;
    char *op_base;
    char *op_suff_from;
    char *op_suff_to;

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

void tacc_target_cg_move_reg_reg(struct tacc_cg_state *state,
                                 uint32_t from,
                                 uint32_t to) {
    char *reg_name;
    char *reg_name_2;

    reg_name = tacc_target_register_name(from, 64);
    reg_name_2 = tacc_target_register_name(to, 64);

    tacc_cg_output(state, "\n\t xchgq %s, %s", reg_name, reg_name_2);
}

void tacc_target_cg_xchg_reg_reg(struct tacc_cg_state *state,
                                 uint32_t reg_a,
                                 uint32_t reg_b) {
    char *reg_name;
    char *reg_name_2;

    reg_name = tacc_target_register_name(reg_a, 64);
    reg_name_2 = tacc_target_register_name(reg_b, 64);

    tacc_cg_output(state, "\n\t xchgq %s, %s", reg_name, reg_name_2);
}

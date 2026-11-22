#include "target/asm.h"
#include "soft_u64.h"
#include "taas.h"
#include "util.h"

struct tacc_asm_target *tacc_asm_target_new(void) {
    struct tacc_asm_target *target;

    target = tacc_malloc(sizeof(struct tacc_asm_target));
    target->at_comments = 0;
    target->hash_comments = 1;
    target->hash_comments_bol = 1;

    return target;
}

#define ASMACT_REG (ASMACT_FIRST_TARGET_SPECIAL + 0)
#define ASMACT_SIB (ASMACT_FIRST_TARGET_SPECIAL + 1)

enum tacc_target_asm_stmt {
    INSTR_MOV_REG,
};

enum tacc_target_asm_operand_form {
    FORM_REG,

    SIB_ABS,
    SIB_ABS_INDEX,
    SIB_PCREL,
    SIB_BASE,
    SIB_BASE_INDEX,
    SIB_ABS_INDEX_32,
    SIB_PCREL_32,
    SIB_BASE_32,
    SIB_BASE_INDEX_32,
};

enum tacc_target_asm_reg {
    ASM_REG_RAX,
    ASM_REG_RBX,
    ASM_REG_RCX,
    ASM_REG_RDX,
    ASM_REG_RSP,
    ASM_REG_RBP,
    ASM_REG_RSI,
    ASM_REG_RDI,
    ASM_REG_R8,
    ASM_REG_R9,
    ASM_REG_R10,
    ASM_REG_R11,
    ASM_REG_R12,
    ASM_REG_R13,
    ASM_REG_R14,
    ASM_REG_R15,
    ASM_REG_AH,
    ASM_REG_CH,
    ASM_REG_DH,
    ASM_REG_BH,
};

static const char *tacc_target_asm_reg_names_8_all[] = {
    "al",   "cl",   "dl",   "bl",   "spl",  "bpl",  "sil", "dil", "r8b", "r9b",
    "r10b", "r11b", "r12b", "r13b", "r14b", "r15b", "ah",  "ch",  "dh",  "bh",
};

static tacc_bool tacc_target_asm_reg8_is_high(uint32_t reg_id) {
    return reg_id >= ASM_REG_AH && reg_id <= ASM_REG_BH;
}
static tacc_bool tacc_target_asm_reg_needs_rex_r8(uint32_t reg_id) {
    return (reg_id >= ASM_REG_RSP && reg_id <= ASM_REG_R15);
}
static tacc_bool tacc_target_asm_reg_needs_rex(uint32_t reg_id) {
    return (reg_id >= ASM_REG_R8 && reg_id <= ASM_REG_R15);
}

static const char *tacc_target_asm_reg_names_16[] = {
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di",
    "r8w",
    "r9w",
    "r10w",
    "r11w",
    "r12w",
    "r13w",
    "r14w",
    "r15w",
};

static const char *tacc_target_asm_reg_names_32[] = {
    "eax",
    "ecx",
    "edx",
    "ebx",
    "esp",
    "ebp",
    "esi",
    "edi",
    "r8d",
    "r9d",
    "r10d",
    "r11d",
    "r12d",
    "r13d",
    "r14d",
    "r15d",
};

static const char *tacc_target_asm_reg_names_64[] = {
    "rax",
    "rcx",
    "rdx",
    "rbx",
    "rsp",
    "rbp",
    "rsi",
    "rdi",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",
};

static tacc_bool tacc_asm_stmt_is_reg(struct tacc_asm_statement *stmt) {
    return tacc_asm_stmt_get_i32(stmt, 0) == FORM_REG;
}

static enum tacc_target_asm_reg
tacc_asm_stmt_get_reg(struct tacc_asm_statement *stmt) {
    tacc_assert(
        ASSERT_ICE, tacc_asm_stmt_is_reg(stmt), "not a register operand");
    return tacc_asm_stmt_get_i32(stmt, 1);
}

static tacc_bool tacc_asm_stmt_is_sib(struct tacc_asm_statement *stmt) {
    return tacc_asm_stmt_get_i32(stmt, 0) >= SIB_ABS &&
           tacc_asm_stmt_get_i32(stmt, 0) <= SIB_BASE_INDEX_32;
}

static void tacc_target_asm_emit_rr(uint32_t width,
                                    struct tacc_asm_statement *from_op,
                                    struct tacc_asm_statement *to_op,
                                    uint8_t base_op,
                                    struct tacc_asm_state *state) {
    uint32_t from;
    uint32_t to;
    uint8_t rex;
    uint8_t modrm;
    uint8_t op;
    tacc_bool has_high;

    from = tacc_asm_stmt_get_reg(from_op);
    to = tacc_asm_stmt_get_reg(to_op);

    rex = 0;
    has_high = 0;
    if (width == 8) {
        if (tacc_target_asm_reg8_is_high(from)) {
            has_high = 1;
            from = (from - ASM_REG_AH) + ASM_REG_RSP;
        } else if (tacc_target_asm_reg_needs_rex(from)) {
            rex = rex | 0x44;
        } else if (tacc_target_asm_reg_needs_rex_r8(from)) {
            rex = rex | 0x40;
        }
        if (tacc_target_asm_reg8_is_high(to)) {
            has_high = 1;
            to = (to - ASM_REG_AH) + ASM_REG_RSP;
        } else if (tacc_target_asm_reg_needs_rex(to)) {
            rex = rex | 0x41;
        } else if (tacc_target_asm_reg_needs_rex_r8(to)) {
            rex = rex | 0x40;
        }
        if (rex != 0) {
            tacc_assert(ASSERT_DIAG,
                        !has_high,
                        "cannot encode high 8-bit registers with REX prefix");
            tacc_asm_out8(state, rex);
        }
        op = base_op;
    } else {
        if (width == 16) {
            tacc_asm_out8(state, 0x66);
        } else if (width == 64) {
            rex = 0x48;
        }
        if (tacc_target_asm_reg_needs_rex(from)) {
            rex = rex | 0x44;
        }
        if (tacc_target_asm_reg_needs_rex(to)) {
            rex = rex | 0x41;
        }
        op = base_op | 1;
        if (rex != 0) {
            tacc_asm_out8(state, rex);
        }
    }
    tacc_asm_out8(state, op);
    modrm = (uint8_t) (0xc0 | (to & 7) | ((from & 7) << 3));
    tacc_asm_out8(state, modrm);
}

static void tacc_target_asm_out32_le(struct tacc_asm_state *state,
                                     struct tacc_asm_symbol *symbol) {
    if (symbol->name == NULL) {
        tacc_asm_out8(state, (uint8_t) (symbol->offset->low));
        tacc_asm_out8(state, (uint8_t) (symbol->offset->low >> 8));
        tacc_asm_out8(state, (uint8_t) (symbol->offset->low >> 16));
        tacc_asm_out8(state, (uint8_t) (symbol->offset->low >> 24));
        return;
    }
    tacc_assert(ASSERT_TODO, 0, "symbol relocations");
}

static void tacc_target_asm_emit_sib_r(uint32_t width,
                                       struct tacc_asm_statement *sib_op,
                                       struct tacc_asm_statement *reg_op,
                                       uint8_t base_op,
                                       struct tacc_asm_state *state) {
    uint32_t reg;
    struct tacc_asm_symbol *displacement;
    uint32_t mod;
    uint32_t rm;
    uint8_t rex;
    struct tacc_asm_symbol *sib_scale;
    uint32_t sib_scale_enc;
    uint32_t sib_index;
    uint32_t sib_base;
    uint8_t op;
    enum tacc_target_asm_operand_form sib_form;
    tacc_bool has_high;
    tacc_bool is_addr_32;
    tacc_bool encode_sib;
    tacc_bool is_zero_disp;

    rex = 0;
    has_high = 0;
    is_addr_32 = 0;
    encode_sib = 1;

    reg = tacc_asm_stmt_get_reg(reg_op);
    sib_form = tacc_asm_stmt_get_i32(sib_op, 0);
    displacement = tacc_asm_stmt_get_symbol(sib_op, 1);
    if (displacement->name == NULL && tacc_u64_is_zero(displacement->offset)) {
        is_zero_disp = 1;
    } else {
        is_zero_disp = 0;
    }

    switch (sib_form) {
    case SIB_ABS:
        mod = 0;
        rm = ASM_REG_RSP;
        sib_scale = NULL;
        sib_base = ASM_REG_RBP;
        sib_index = ASM_REG_RSP;
        break;

    case SIB_ABS_INDEX_32:
        is_addr_32 = 1;
        /* fallthrough */
    case SIB_ABS_INDEX:
        mod = 0;
        rm = ASM_REG_RSP;
        sib_base = ASM_REG_RBP;
        sib_index =
            (uint8_t) tacc_asm_stmt_get_reg(tacc_asm_stmt_get_sub(sib_op, 2));
        tacc_assert(
            ASSERT_DIAG, sib_index != ASM_REG_RSP, "cannot use rsp as index");
        sib_scale = tacc_asm_stmt_get_symbol(sib_op, 3);
        break;

    case SIB_PCREL_32:
        is_addr_32 = 1;
        /* fallthrough */
    case SIB_PCREL:
        mod = 0;
        rm = ASM_REG_RBP;
        encode_sib = 0;
        break;

    case SIB_BASE_32:
        is_addr_32 = 1;
        /* fallthrough */
    case SIB_BASE:
        rm = (uint8_t) tacc_asm_stmt_get_reg(tacc_asm_stmt_get_sub(sib_op, 2));
        if (rm == ASM_REG_RBP || rm == ASM_REG_R13 || !is_zero_disp) {
            mod = 2;
        } else if (is_zero_disp) {
            mod = 0;
            displacement = NULL;
        }
        if (rm == ASM_REG_R12 || rm == ASM_REG_RSP) {
            sib_base = rm;
            sib_index = ASM_REG_RSP;
            sib_scale = NULL;
            /* transform to SIB form rather than rm form */
            rm = ASM_REG_RSP;
        } else {
            encode_sib = 0;
        }
        break;

    case SIB_BASE_INDEX_32:
        is_addr_32 = 1;
        /* fallthrough */
    case SIB_BASE_INDEX:
        if (is_zero_disp) {
            mod = 0;
            displacement = NULL;
        } else {
            mod = 2;
        }
        rm = ASM_REG_RSP;
        sib_base =
            (uint8_t) tacc_asm_stmt_get_reg(tacc_asm_stmt_get_sub(sib_op, 2));
        sib_index =
            (uint8_t) tacc_asm_stmt_get_reg(tacc_asm_stmt_get_sub(sib_op, 3));
        tacc_assert(
            ASSERT_DIAG, sib_index != ASM_REG_RSP, "cannot use rsp as index");
        sib_scale = tacc_asm_stmt_get_symbol(sib_op, 4);
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "form_reg as sib operand");
        break;
    }

    if (is_addr_32) {
        /* address size override */
        tacc_asm_out8(state, 0x67);
    }

    if (width == 8) {
        if (tacc_target_asm_reg8_is_high(reg)) {
            has_high = 1;
            reg = (reg - ASM_REG_AH) + ASM_REG_RSP;
        } else if (tacc_target_asm_reg_needs_rex(reg)) {
            rex = rex | 0x44;
        } else if (tacc_target_asm_reg_needs_rex_r8(reg)) {
            rex = rex | 0x40;
        }
        op = base_op;
    } else {
        if (width == 16) {
            tacc_asm_out8(state, 0x66);
        } else if (width == 64) {
            rex = 0x48;
        }
        if (tacc_target_asm_reg_needs_rex(reg)) {
            rex = rex | 0x44;
        }
        op = base_op | 1;
    }

    if (encode_sib) {
        if (tacc_target_asm_reg_needs_rex(sib_index)) {
            rex = rex | 0x42;
        }
        if (tacc_target_asm_reg_needs_rex(sib_base)) {
            rex = rex | 0x41;
        }
    }

    tacc_assert(ASSERT_DIAG,
                rex == 0 || !has_high,
                "cannot encode high register with REX prefix");
    if (rex != 0) {
        tacc_asm_out8(state, rex);
    }

    tacc_asm_out8(state, op);
    tacc_asm_out8(state, (uint8_t) ((mod << 6) | (rm & 7) | ((reg & 7) << 3)));
    if (encode_sib) {
        if (sib_scale != NULL) {
            tacc_assert(ASSERT_DIAG,
                        sib_scale->name == NULL,
                        "cannot use relocation in scale factor");
            switch (sib_scale->offset->low) {
            case 1:
                sib_scale_enc = 0;
                break;
            case 2:
                sib_scale_enc = 1;
                break;
            case 4:
                sib_scale_enc = 2;
                break;
            case 8:
                sib_scale_enc = 3;
                break;
            default:
                tacc_assert(
                    ASSERT_DIAG, 0, "invalid scaling factor %d", sib_scale);
                break;
            }
        } else {
            sib_scale_enc = 0;
        }
        tacc_asm_out8(state,
                      (uint8_t) ((sib_scale_enc << 6) | ((sib_index & 7) << 3) |
                                 (sib_base & 7)));
    }
    if (displacement != NULL) {
        tacc_target_asm_out32_le(state, displacement);
    }
}

static void tacc_target_asm_emit_modrm(struct tacc_asm_statement *stmt,
                                       uint8_t base_op,
                                       struct tacc_asm_state *state) {
    struct tacc_asm_statement *from_op;
    struct tacc_asm_statement *to_op;
    uint32_t width;

    width = tacc_asm_stmt_get_i32(stmt, 1);
    from_op = tacc_asm_stmt_get_sub(stmt, 2);
    to_op = tacc_asm_stmt_get_sub(stmt, 3);

    if (tacc_asm_stmt_is_reg(from_op) && tacc_asm_stmt_is_reg(to_op)) {
        tacc_target_asm_emit_rr(width, from_op, to_op, base_op, state);
    } else if (tacc_asm_stmt_is_sib(from_op)) {
        tacc_target_asm_emit_sib_r(width, from_op, to_op, base_op | 2, state);
    } else if (tacc_asm_stmt_is_sib(to_op)) {
        tacc_target_asm_emit_sib_r(width, to_op, from_op, base_op, state);
    } else {
        tacc_assert(ASSERT_ICE, 0, "I don't know this kind of modrm");
    }
}

void tacc_target_asm_process_statement(struct tacc_asm_statement *stmt,
                                       struct tacc_asm_state *state) {
    uint32_t kind;

    kind = tacc_asm_stmt_get_i32(stmt, 0);
    switch ((enum tacc_target_asm_stmt) kind) {
    case INSTR_MOV_REG:
        tacc_target_asm_emit_modrm(stmt, 0x88, state);
        break;
    default:
        tacc_assert(ASSERT_ICE, 0, "I don't know this instruction...");
        break;
    }
}

tacc_bool tacc_target_asm_part_match(struct tacc_asm_statement *stmt,
                                     struct tacc_asm_match_action *action,
                                     struct tacc_file_iter *iter,
                                     struct tacc_asm_state *state) {
    TACC_UNUSED(stmt);
    TACC_UNUSED(action);
    TACC_UNUSED(iter);
    TACC_UNUSED(state);

    tacc_assert(ASSERT_TODO, 0, "asm part match");
}

void tacc_target_asm_statement_part_free(
    struct tacc_target_asm_statement_part *part) {
    TACC_UNUSED(part);

    tacc_assert(ASSERT_TODO, 0, "asm statement part free");
}

void tacc_target_asm_match_action_free(
    struct tacc_target_asm_match_action *action) {
    TACC_UNUSED(action);
}

struct tacc_target_asm_actions {
    struct tacc_asm_match_action *r8;
    struct tacc_asm_match_action *r16;
    struct tacc_asm_match_action *r32;
    struct tacc_asm_match_action *r64;
    struct tacc_asm_match_action *comma;
    struct tacc_asm_match_action *i8;
    struct tacc_asm_match_action *i16;
    struct tacc_asm_match_action *i32;
    struct tacc_asm_match_action *i64;
};

static void tacc_target_asm_form_2(struct tacc_asm_matcher *matcher,
                                   const char *key_suffixed,
                                   const char *key_unsuffixed,
                                   const char *template,
                                   ...) {
    va_list va;
    va_start(va, template);

    tacc_asm_vform(matcher, key_suffixed, template, va);

    va_end(va);

    va_start(va, template);

    tacc_asm_vform(matcher, key_unsuffixed, template, va);

    va_end(va);
}

static void tacc_target_asm_form_bwlq_rr(struct tacc_asm_matcher *matcher,
                                         uint32_t insn,
                                         const char *key,
                                         const char *key_b,
                                         const char *key_w,
                                         const char *key_l,
                                         const char *key_q) {
    tacc_target_asm_form_2(
        matcher, key_b, key, "%d %d %?, %?", insn, 8, "@reg8", "@reg8");
    tacc_target_asm_form_2(
        matcher, key_w, key, "%d %d %?, %?", insn, 16, "@reg16", "@reg16");
    tacc_target_asm_form_2(
        matcher, key_l, key, "%d %d %?, %?", insn, 32, "@reg32", "@reg32");
    tacc_target_asm_form_2(
        matcher, key_q, key, "%d %d %?, %?", insn, 64, "@reg64", "@reg64");
}
static void tacc_target_asm_form_bwlq_r_sib(struct tacc_asm_matcher *matcher,
                                            uint32_t insn,
                                            const char *key,
                                            const char *key_b,
                                            const char *key_w,
                                            const char *key_l,
                                            const char *key_q) {
    tacc_target_asm_form_2(
        matcher, key_b, key, "%d %d %?, %?", insn, 8, "@reg8", "@sib");
    tacc_target_asm_form_2(
        matcher, key_w, key, "%d %d %?, %?", insn, 16, "@reg16", "@sib");
    tacc_target_asm_form_2(
        matcher, key_l, key, "%d %d %?, %?", insn, 32, "@reg32", "@sib");
    tacc_target_asm_form_2(
        matcher, key_q, key, "%d %d %?, %?", insn, 64, "@reg64", "@sib");
}
static void tacc_target_asm_form_bwlq_sib_r(struct tacc_asm_matcher *matcher,
                                            uint32_t insn,
                                            const char *key,
                                            const char *key_b,
                                            const char *key_w,
                                            const char *key_l,
                                            const char *key_q) {
    tacc_target_asm_form_2(
        matcher, key_b, key, "%d %d %?, %?", insn, 8, "@sib", "@reg8");
    tacc_target_asm_form_2(
        matcher, key_w, key, "%d %d %?, %?", insn, 16, "@sib", "@reg16");
    tacc_target_asm_form_2(
        matcher, key_l, key, "%d %d %?, %?", insn, 32, "@sib", "@reg32");
    tacc_target_asm_form_2(
        matcher, key_q, key, "%d %d %?, %?", insn, 64, "@sib", "@reg64");
}

#define GEN_BWLQS(base_name)                                              \
    base_name, base_name "b", base_name "w", base_name "l", base_name "q"

void tacc_target_asm_init_matchers(struct tacc_asm_matcher *matcher) {
    size_t i;

    for (i = 0; i < 20; i = i + 1) {
        tacc_asm_form(matcher,
                      "@reg8",
                      "%% %d %s %d",
                      FORM_REG,
                      tacc_target_asm_reg_names_8_all[i],
                      i);

        if (i >= 16) {
            continue;
        }

        tacc_asm_form(matcher,
                      "@reg16",
                      "%% %d %s %d",
                      FORM_REG,
                      tacc_target_asm_reg_names_16[i],
                      i);
        tacc_asm_form(matcher,
                      "@reg32",
                      "%% %d %s %d",
                      FORM_REG,
                      tacc_target_asm_reg_names_32[i],
                      i);
        tacc_asm_form(matcher,
                      "@reg64",
                      "%% %d %s %d",
                      FORM_REG,
                      tacc_target_asm_reg_names_64[i],
                      i);
    }

    tacc_asm_form(matcher, "@sib", "%d %e(%?)", SIB_BASE, 0, "@reg64");
    tacc_asm_form(matcher, "@sib", "%d %e(%?)", SIB_BASE_32, 0, "@reg32");
    tacc_asm_form(matcher,
                  "@sib",
                  "%d %e(%?,%?) %d",
                  SIB_BASE_INDEX,
                  0,
                  "@reg64",
                  "@reg64",
                  1);
    tacc_asm_form(matcher,
                  "@sib",
                  "%d %e(%?,%?) %d",
                  SIB_BASE_INDEX_32,
                  0,
                  "@reg32",
                  "@reg32",
                  1);
    tacc_asm_form(matcher,
                  "@sib",
                  "%d %e(%?,%?,%e)",
                  SIB_BASE_INDEX,
                  0,
                  "@reg64",
                  "@reg64",
                  1);
    tacc_asm_form(matcher,
                  "@sib",
                  "%d %e(%?,%?,%e)",
                  0,
                  SIB_BASE_INDEX_32,
                  "@reg64",
                  "@reg64",
                  1);
    tacc_asm_form(
        matcher, "@sib", "%d %e(,%?,%e)", SIB_ABS_INDEX, 0, "@reg64", 1);
    tacc_asm_form(
        matcher, "@sib", "%d %e(,%?,%e)", SIB_ABS_INDEX_32, 0, "@reg32", 1);
    tacc_asm_form(matcher, "@sib", "%d %e(%% rip)", SIB_PCREL, 0);
    tacc_asm_form(matcher, "@sib", "%d %e(%% eip)", SIB_PCREL_32, 0);
    tacc_asm_form(
        matcher, "@sib", "%d %e(,%?,%e)", SIB_ABS_INDEX_32, 0, "@reg32", 1);
    tacc_asm_form(matcher, "@sib", "%d %E", SIB_ABS);

    tacc_target_asm_form_bwlq_rr(matcher, INSTR_MOV_REG, GEN_BWLQS("mov"));
    tacc_target_asm_form_bwlq_r_sib(matcher, INSTR_MOV_REG, GEN_BWLQS("mov"));
    tacc_target_asm_form_bwlq_sib_r(matcher, INSTR_MOV_REG, GEN_BWLQS("mov"));
}

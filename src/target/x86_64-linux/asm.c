#include "target/asm.h"
#include "taas.h"

struct tacc_asm_target *tacc_asm_target_new(void) {
    struct tacc_asm_target *target;

    target = tacc_malloc(sizeof(struct tacc_asm_target));
    target->at_comments = 0;
    target->hash_comments = 1;
    target->hash_comments_bol = 1;

    return target;
}

#define ASMACT_REG (ASMACT_FIRST_TARGET_SPECIAL + 0)

enum tacc_target_asm_stmt {
    INSTR_MOV_REG,
};

enum tacc_target_asm_reg {
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSP,
    REG_RBP,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_AH,
    REG_CH,
    REG_DH,
    REG_BH,
};

static const char *tacc_target_asm_reg_names_8_all[] = {
    "al",   "bl",   "cl",   "dl",   "spl",  "bpl",  "sil", "dil", "r8b", "r9b",
    "r10b", "r11b", "r12b", "r13b", "r14b", "r15b", "ah",  "ch",  "dh",  "bh",
};

static tacc_bool tacc_target_asm_reg8_is_high(uint32_t reg_id) {
    return reg_id >= REG_AH && reg_id <= REG_BH;
}
static tacc_bool tacc_target_asm_reg_needs_rex_r8(uint32_t reg_id) {
    return (reg_id >= REG_RSP && reg_id <= REG_R15);
}
static tacc_bool tacc_target_asm_reg_needs_rex(uint32_t reg_id) {
    return (reg_id >= REG_R8 && reg_id <= REG_R15);
}

static const char *tacc_target_asm_reg_names_16[] = {
    "ax",
    "bx",
    "cx",
    "dx",
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
    "ebx",
    "ecx",
    "edx",
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
    "rbx",
    "rcx",
    "rdx",
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

struct tacc_target_asm_match_action {
    struct tacc_asm_match_action *sub_action;
};

static struct tacc_asm_match_action *
tacc_target_reg_matcher(const char **options, size_t count) {
    struct tacc_asm_match_action *sub_action;
    struct tacc_asm_match_action *target_action;

    sub_action = tacc_asm_enum(options, count);
    target_action = tacc_asm_match_action_new();
    target_action->kind = ASMACT_REG;
    target_action->extra.target_data =
        tacc_malloc(sizeof(struct tacc_target_asm_match_action));
    target_action->extra.target_data->sub_action = sub_action;

    return target_action;
}

tacc_bool tacc_target_asm_part_match(struct tacc_asm_statement *stmt,
                                     struct tacc_asm_match_action *action,
                                     struct tacc_asm_file_iter *iter,
                                     struct tacc_asm_state *state) {
    TACC_UNUSED(state);

    tacc_assert(ASSERT_ICE,
                action->kind == ASMACT_REG,
                "I thought reg was the only target-specific matcher for us");

    tacc_asm_file_iter_eat_ws_no_newlines(iter);
    if (!tacc_asm_file_iter_accept_tok(iter, "%")) {
        return 0;
    }
    tacc_asm_file_iter_eat_ws_no_newlines(iter);

    return tacc_asm_part_match(
        stmt, action->extra.target_data->sub_action, iter, state);
}

static void tacc_target_asm_process_modrm(struct tacc_asm_statement *stmt,
                                          uint8_t base_op,
                                          struct tacc_asm_state *state) {
    uint32_t width;
    uint32_t from;
    uint32_t to;
    uint8_t rex;
    uint8_t modrm;
    uint8_t op;
    tacc_bool has_high;

    width = tacc_asm_stmt_get_i32(stmt, 1);
    from = tacc_asm_stmt_get_i32(stmt, 2);
    to = tacc_asm_stmt_get_i32(stmt, 3);

    rex = 0;
    has_high = 0;
    if (width == 8) {
        if (tacc_target_asm_reg8_is_high(from)) {
            has_high = 1;
            from = (from - REG_AH) + REG_RSP;
        } else if (tacc_target_asm_reg_needs_rex_r8(from)) {
            rex = rex | 0x04;
        }
        if (tacc_target_asm_reg8_is_high(to)) {
            has_high = 1;
            to = (to - REG_AH) + REG_RSP;
        } else if (tacc_target_asm_reg_needs_rex_r8(to)) {
            rex = rex | 0x01;
        }
        op = base_op;
    } else {
        if (width == 16) {
            tacc_asm_out8(state, 0x66);
        } else if (width == 64) {
            rex = 0x08;
        }
        if (tacc_target_asm_reg_needs_rex(from)) {
            rex = rex | 0x04;
        }
        if (tacc_target_asm_reg_needs_rex(to)) {
            rex = rex | 0x01;
        }
        op = base_op | 1;
    }
    if (rex != 0) {
        tacc_assert(ASSERT_DIAG,
                    !has_high,
                    "cannot encode high 8-bit registers with REX prefix");
        tacc_asm_out8(state, rex | 0x40);
    }
    tacc_asm_out8(state, op);
    modrm = (uint8_t) (0xc0 | (to & 7) | ((from & 7) << 3));
    tacc_asm_out8(state, modrm);
}

void tacc_target_asm_process_statement(struct tacc_asm_statement *stmt,
                                       struct tacc_asm_state *state) {
    uint32_t kind;

    kind = tacc_asm_stmt_get_i32(stmt, 0);
    switch ((enum tacc_target_asm_stmt) kind) {
    case INSTR_MOV_REG:
        tacc_target_asm_process_modrm(stmt, 0x88, state);
        break;
    }
}

void tacc_target_asm_statement_part_free(
    struct tacc_target_asm_statement_part *part) {
    TACC_UNUSED(part);

    tacc_assert(ASSERT_TODO, 0, "asm statement part free");
}

void tacc_target_asm_match_action_free(
    struct tacc_target_asm_match_action *action) {
    tacc_asm_match_action_free(action->sub_action);
    tacc_free(action);
}

#define S tacc_asm_strtok
#define I tacc_asm_imm32
struct tacc_asm_match_action_list *
tacc_target_asm_init_matchers(struct tacc_asm_matcher *matcher) {
    struct tacc_asm_match_action_list *m;
    struct tacc_asm_match_action *r8;
    struct tacc_asm_match_action *r8unrex;
    struct tacc_asm_match_action *r8rex;
    struct tacc_asm_match_action *r16;
    struct tacc_asm_match_action *r32;
    struct tacc_asm_match_action *r64;
    struct tacc_asm_match_action *comma;
    struct tacc_asm_match_action *i8;
    struct tacc_asm_match_action *i16;
    struct tacc_asm_match_action *i32;
    struct tacc_asm_match_action *i64;
    struct tacc_asm_match_action *insn;

    m = tacc_asm_match_action_list_new();
    r8 = tacc_target_reg_matcher(tacc_target_asm_reg_names_8_all, 20);
    r16 = tacc_target_reg_matcher(tacc_target_asm_reg_names_16, 16);
    r32 = tacc_target_reg_matcher(tacc_target_asm_reg_names_32, 16);
    r64 = tacc_target_reg_matcher(tacc_target_asm_reg_names_64, 16);
    tacc_asm_match_action_list_push(m, r8);
    tacc_asm_match_action_list_push(m, r16);
    tacc_asm_match_action_list_push(m, r32);
    tacc_asm_match_action_list_push(m, r64);
    i8 = I(8);
    i16 = I(16);
    i32 = I(32);
    i64 = I(64);
    tacc_asm_match_action_list_push(m, i8);
    tacc_asm_match_action_list_push(m, i16);
    tacc_asm_match_action_list_push(m, i32);
    tacc_asm_match_action_list_push(m, i64);
    comma = S(",");
    tacc_asm_match_action_list_push(m, comma);

    TACC_UNUSED(r8unrex);
    TACC_UNUSED(r8rex);

    insn = I(INSTR_MOV_REG);
    tacc_asm_match_action_list_push(m, insn);
    tacc_asm_form(matcher, "movb", insn, i8, r8, comma, r8, NULL);
    tacc_asm_form(matcher, "mov", insn, i8, r8, comma, r8, NULL);
    tacc_asm_form(matcher, "movw", insn, i16, r16, comma, r16, NULL);
    tacc_asm_form(matcher, "mov", insn, i16, r16, comma, r16, NULL);
    tacc_asm_form(matcher, "movl", insn, i32, r32, comma, r32, NULL);
    tacc_asm_form(matcher, "mov", insn, i32, r32, comma, r32, NULL);
    tacc_asm_form(matcher, "movq", insn, i64, r64, comma, r64, NULL);
    tacc_asm_form(matcher, "mov", insn, i64, r64, comma, r64, NULL);

    return m;
}
#undef S

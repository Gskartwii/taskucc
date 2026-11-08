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

static const char *tacc_target_asm_reg_names_8_rex[] = {
    "al",
    "bl",
    "cl",
    "dl",
    "spl",
    "bpl",
    "sil",
    "dil",
    "r8b",
    "r9b",
    "r10b",
    "r11b",
    "r12b",
    "r13b",
    "r14b",
    "r15b",
};
static const char *tacc_target_asm_reg_names_8_all[] = {
    "al",   "bl",   "cl",   "dl",   "spl",  "bpl",  "sil", "dil", "r8b", "r9b",
    "r10b", "r11b", "r12b", "r13b", "r14b", "r15b", "ah",  "ch",  "dh",  "bh",
};
static const char *tacc_target_asm_reg_names_8_unrex[] = {
    "al",
    "bl",
    "cl",
    "dl",
    "ah",
    "ch",
    "dh",
    "bh",
    "r8b",
    "r9b",
    "r10b",
    "r11b",
    "r12b",
    "r13b",
    "r14b",
    "r15b",
};

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

static void tacc_target_asm_process_movreg(struct tacc_asm_statement *stmt,
                                           struct tacc_asm_state *state) {
    uint32_t width;
    uint32_t from;
    uint32_t to;

    width = tacc_asm_stmt_get_i32(stmt, 1);
    from = tacc_asm_stmt_get_i32(stmt, 2);
    to = tacc_asm_stmt_get_i32(stmt, 3);

    fprintf(state->out_file, "move %d bits from %d to %d\n", width, from, to);
}

void tacc_target_asm_process_statement(struct tacc_asm_statement *stmt,
                                       struct tacc_asm_state *state) {
    uint32_t kind;

    kind = tacc_asm_stmt_get_i32(stmt, 0);
    switch ((enum tacc_target_asm_stmt) kind) {
    case INSTR_MOV_REG:
        tacc_target_asm_process_movreg(stmt, state);
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

    if (action->sub_action != NULL) {
        tacc_asm_match_action_free(action->sub_action);
    }
    tacc_free(action);
}

#define S tacc_asm_strtok
#define I tacc_asm_imm32
void tacc_target_asm_init_matchers(struct tacc_asm_matcher *matcher) {
    struct tacc_asm_match_action *r8;
    struct tacc_asm_match_action *r8unrex;
    struct tacc_asm_match_action *r8rex;
    struct tacc_asm_match_action *r16;
    struct tacc_asm_match_action *r32;
    struct tacc_asm_match_action *r64;
    struct tacc_asm_match_action *comma;

    r8unrex = tacc_target_reg_matcher(tacc_target_asm_reg_names_8_unrex, 16);
    r8rex = tacc_target_reg_matcher(tacc_target_asm_reg_names_8_rex, 16);
    r8 = tacc_target_reg_matcher(tacc_target_asm_reg_names_8_all, 20);
    r16 = tacc_target_reg_matcher(tacc_target_asm_reg_names_16, 16);
    r32 = tacc_target_reg_matcher(tacc_target_asm_reg_names_32, 16);
    r64 = tacc_target_reg_matcher(tacc_target_asm_reg_names_64, 16);
    comma = S(",");

    TACC_UNUSED(r8unrex);
    TACC_UNUSED(r8rex);

    tacc_asm_form(matcher, "movb", I(INSTR_MOV_REG), I(8), r8, comma, r8, NULL);
    tacc_asm_form(matcher, "mov", I(INSTR_MOV_REG), I(8), r8, comma, r8, NULL);
    tacc_asm_form(
        matcher, "movw", I(INSTR_MOV_REG), I(16), r16, comma, r16, NULL);
    tacc_asm_form(
        matcher, "mov", I(INSTR_MOV_REG), I(16), r16, comma, r16, NULL);
    tacc_asm_form(
        matcher, "movl", I(INSTR_MOV_REG), I(32), r32, comma, r32, NULL);
    tacc_asm_form(
        matcher, "mov", I(INSTR_MOV_REG), I(32), r32, comma, r32, NULL);
    tacc_asm_form(
        matcher, "movq", I(INSTR_MOV_REG), I(64), r64, comma, r64, NULL);
    tacc_asm_form(
        matcher, "mov", I(INSTR_MOV_REG), I(64), r64, comma, r64, NULL);
}
#undef S

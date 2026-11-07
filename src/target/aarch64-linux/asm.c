#include "target/asm.h"

struct tacc_asm_target *tacc_asm_target_new(void) {
    struct tacc_asm_target *target;

    target = tacc_malloc(sizeof(struct tacc_asm_target));
    target->at_comments = 0;
    target->hash_comments = 0;
    target->hash_comments_bol = 1;

    return target;
}

tacc_bool tacc_target_asm_part_match(struct tacc_asm_statement *stmt,
                                     struct tacc_asm_match_action *action,
                                     struct tacc_asm_file_iter *iter,
                                     struct tacc_asm_state *state) {
    TACC_UNUSED(stmt);
    TACC_UNUSED(action);
    TACC_UNUSED(iter);
    TACC_UNUSED(state);

    tacc_assert(ASSERT_TODO, 0, "asm part match");
}

void tacc_target_asm_process_statement(struct tacc_asm_statement *stmt,
                                       struct tacc_asm_state *state) {
    TACC_UNUSED(stmt);
    TACC_UNUSED(state);

    tacc_assert(ASSERT_TODO, 0, "asm process statement");
}

void tacc_target_asm_statement_part_free(
    struct tacc_target_asm_statement_part *part) {
    TACC_UNUSED(part);

    tacc_assert(ASSERT_TODO, 0, "asm statement part free");
}

void tacc_target_asm_match_action_free(
    struct tacc_target_asm_match_action *action) {
    TACC_UNUSED(action);

    tacc_assert(ASSERT_TODO, 0, "asm match action free");
}

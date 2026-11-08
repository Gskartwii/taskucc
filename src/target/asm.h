#ifndef TACC_TARGET_ASM_H
#define TACC_TARGET_ASM_H

#include "taas.h"
#include "util.h"

struct tacc_asm_target {
    tacc_bool at_comments;
    tacc_bool hash_comments;
    tacc_bool hash_comments_bol;
};

struct tacc_asm_target *tacc_asm_target_new(void);
tacc_bool tacc_target_asm_part_match(struct tacc_asm_statement *stmt,
                                     struct tacc_asm_match_action *action,
                                     struct tacc_asm_file_iter *iter,
                                     struct tacc_asm_state *state);
void tacc_target_asm_process_statement(struct tacc_asm_statement *stmt,
                                       struct tacc_asm_state *state);
void tacc_target_asm_init_matchers(struct tacc_asm_matcher *matcher);
void tacc_target_asm_statement_part_free(
    struct tacc_target_asm_statement_part *part);
void tacc_target_asm_match_action_free(
    struct tacc_target_asm_match_action *action);

#endif

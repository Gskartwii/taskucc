#ifndef TACC_TAAS_H
#define TACC_TAAS_H

#include "dynarray.h"
#include "dynhash.h"
#include "tasku_file.h"
#include <stdio.h>

enum tacc_asm_match_action_kind {
    ASMACT_STRTOK,
    ASMACT_MAYBE_STRTOK,
    ASMACT_EXPR,
    ASMACT_PUSH_IMM32,
    ASMACT_PUSH_IMM64,
    ASMACT_EXPECT_LINESEP,
    ASMACT_FIRST_TARGET_SPECIAL,
};

struct tacc_asm_match_action {
    enum tacc_asm_match_action_kind kind;
    union {
        /* STRTOK, MAYBE_STRTOK */
        char *str_tok;

        /* PUSH_IMM32 */
        uint32_t imm32;

        /* PUSH_IMM64 */
        struct tacc_u64 *imm64;

        /* TARGET_SPECIAL */
        struct tacc_target_asm_match_action *target_data;
    } extra;
};

DECL_DYNARRAY_OVER(tacc_asm_match_action_list,
                   tacc_asm_match_action_list_entry,
                   struct tacc_asm_match_action *,
                   tacc_asm_match_action_list_new,
                   tacc_asm_match_action_list_init,
                   tacc_asm_match_action_list_get,
                   tacc_asm_match_action_list_push,
                   tacc_asm_match_action_list_pop,
                   tacc_asm_match_action_list_len,
                   tacc_asm_match_action_list_free)

struct tacc_asm_candidate {
    struct tacc_asm_match_action_list *actions;
};

DECL_DYNARRAY_OVER(tacc_asm_candidate_list,
                   tacc_asm_candidate_list_entry,
                   struct tacc_asm_candidate *,
                   tacc_asm_candidate_list_new,
                   tacc_asm_candidate_list_init,
                   tacc_asm_candidate_list_get,
                   tacc_asm_candidate_list_push,
                   tacc_asm_candidate_list_pop,
                   tacc_asm_candidate_list_len,
                   tacc_asm_candidate_list_free)

struct tacc_asm_candidates {
    char *key;
    struct tacc_asm_candidate_list *list;
};

DECL_DYNHASH_OVER(tacc_asm_candidates_map,
                  tacc_asm_candidates_map_entry,
                  struct tacc_asm_candidates *,
                  tacc_asm_candidates_map_new,
                  tacc_asm_candidates_map_init,
                  tacc_asm_candidates_map_get,
                  tacc_asm_candidates_map_insert,
                  tacc_asm_candidates_map_fill_count,
                  tacc_asm_candidates_map_free)

struct tacc_asm_matcher {
    struct tacc_asm_candidates_map *matchers;
};

enum tacc_asm_statement_part_kind {
    ASMPART_IMM32,
    ASMPART_IMM64,
    ASMPART_TARGET,
};

struct tacc_asm_statement_part {
    enum tacc_asm_statement_part_kind kind;

    union {
        uint32_t imm32;
        struct tacc_u64 *imm64;
        struct tacc_target_asm_statement_part *target_data;
    } extra;
};

DECL_DYNARRAY_OVER(tacc_asm_statement_part_list,
                   tacc_asm_statement_part_list_entry,
                   struct tacc_asm_statement_part *,
                   tacc_asm_statement_part_list_new,
                   tacc_asm_statement_part_list_init,
                   tacc_asm_statement_part_list_get,
                   tacc_asm_statement_part_list_push,
                   tacc_asm_statement_part_list_pop,
                   tacc_asm_statement_part_list_len,
                   tacc_asm_statement_part_list_free)

struct tacc_asm_statement {
    struct tacc_asm_statement_part_list *parts;
};

struct tacc_asm_file_iter {
    struct tacc_asm_target *target;

    /* owning */
    char *orig;
    /* borrow of orig */
    char *src;
    /* borrow of src */
    char *end;
    /* owning, possibly null */
    char *filename;
};

struct tacc_asm_state {
    FILE *out_file;
    struct tacc_asm_matcher *matcher;
};

struct tacc_asm_statement *tacc_asm_statement_new(void);
void tacc_asm_statement_free(struct tacc_asm_statement *statement);
void tacc_asm_match_action_free(struct tacc_asm_match_action *action);
void tacc_asm_candidate_free(struct tacc_asm_candidate *candidate);
void tacc_asm_candidates_free(struct tacc_asm_candidates *candidates);
void tacc_asm_statement_part_free(struct tacc_asm_statement_part *part);
struct tacc_asm_statement_part *tacc_asm_statement_part_new(void);
void taas_assemble(struct tacc_file *in_file, FILE *out_file);

#endif

#include "taas.h"
#include "dynstring.h"
#include "target/asm.h"
#include "util.h"

MK_DYNARRAY_OVER(tacc_asm_match_action_list,
                 tacc_asm_match_action_list_entry,
                 struct tacc_asm_match_action *,
                 tacc_asm_match_action_list_new,
                 tacc_asm_match_action_list_init,
                 tacc_asm_match_action_list_get,
                 tacc_asm_match_action_list_push,
                 tacc_asm_match_action_list_pop,
                 tacc_asm_match_action_list_len,
                 tacc_asm_match_action_free,
                 tacc_asm_match_action_list_free)

MK_DYNARRAY_OVER(tacc_asm_candidate_list,
                 tacc_asm_candidate_list_entry,
                 struct tacc_asm_candidate *,
                 tacc_asm_candidate_list_new,
                 tacc_asm_candidate_list_init,
                 tacc_asm_candidate_list_get,
                 tacc_asm_candidate_list_push,
                 tacc_asm_candidate_list_pop,
                 tacc_asm_candidate_list_len,
                 tacc_asm_candidate_free,
                 tacc_asm_candidate_list_free)

MK_DYNHASH_OVER(tacc_asm_candidates_map,
                key,
                tacc_asm_candidates_map_entry,
                struct tacc_asm_candidates *,
                tacc_asm_candidates_map_new,
                tacc_asm_candidates_map_init,
                tacc_asm_candidates_map_get,
                tacc_asm_candidates_map_insert,
                tacc_asm_candidates_map_fill_count,
                tacc_asm_candidates_free,
                tacc_asm_candidates_map_free)

MK_DYNARRAY_OVER(tacc_asm_statement_part_list,
                 tacc_asm_statement_part_list_entry,
                 struct tacc_asm_statement_part *,
                 tacc_asm_statement_part_list_new,
                 tacc_asm_statement_part_list_init,
                 tacc_asm_statement_part_list_get,
                 tacc_asm_statement_part_list_push,
                 tacc_asm_statement_part_list_pop,
                 tacc_asm_statement_part_list_len,
                 tacc_asm_statement_part_free,
                 tacc_asm_statement_part_list_free)

struct tacc_asm_statement_part *tacc_asm_statement_part_new(void) {
    struct tacc_asm_statement_part *part;

    part = tacc_malloc(sizeof(struct tacc_asm_statement_part));
    part->kind = ASMPART_IMM32;
    part->extra.imm32 = 0;

    return part;
}

struct tacc_asm_statement *tacc_asm_statement_new(void) {
    struct tacc_asm_statement *stmt;

    stmt = tacc_malloc(sizeof(struct tacc_asm_statement));
    stmt->parts = tacc_asm_statement_part_list_new();

    return stmt;
}

void tacc_asm_statement_part_free(struct tacc_asm_statement_part *part) {
    switch (part->kind) {
    case ASMPART_IMM32:
        break;
    case ASMPART_IMM64:
        tacc_free(part->extra.imm64);
        break;
    case ASMPART_TARGET:
        tacc_target_asm_statement_part_free(part->extra.target_data);
        break;
    }
    tacc_free(part);
}

void tacc_asm_match_action_free(struct tacc_asm_match_action *action) {
    switch (action->kind) {
    case ASMACT_STRTOK:
    case ASMACT_MAYBE_STRTOK:
    case ASMACT_EXPR:
    case ASMACT_PUSH_IMM32:
    case ASMACT_EXPECT_LINESEP:
        break;
    case ASMACT_PUSH_IMM64:
        tacc_free(action->extra.imm64);
        break;
    case ASMACT_FIRST_TARGET_SPECIAL:
        tacc_target_asm_match_action_free(action->extra.target_data);
        break;
    }
    tacc_free(action);
}

void tacc_asm_candidate_free(struct tacc_asm_candidate *candidate) {
    tacc_asm_match_action_list_free(candidate->actions);
    tacc_free(candidate->actions);
    tacc_free(candidate);
}

void tacc_asm_candidates_free(struct tacc_asm_candidates *candidates) {
    tacc_asm_candidate_list_free(candidates->list);
    tacc_free(candidates->list);
    tacc_free(candidates);
}

void tacc_asm_file_iter_free(struct tacc_asm_file_iter *iter) {
    if (iter->filename) {
        tacc_free(iter->filename);
    }
    tacc_free(iter->orig);
    tacc_free(iter);
}

void tacc_asm_statement_free(struct tacc_asm_statement *statement) {
    tacc_asm_statement_part_list_free(statement->parts);
    tacc_free(statement->parts);
    tacc_free(statement);
}

struct tacc_asm_file_iter *tacc_asm_file_iter_new_file(
    struct tacc_file *file, struct tacc_asm_target *target) {
    struct tacc_asm_file_iter *iter;

    iter = tacc_malloc(sizeof(struct tacc_asm_file_iter));
    iter->filename = file->name;
    iter->src = file->src;
    iter->orig = file->src;
    iter->end = file->src + file->len;
    iter->target = target;

    return iter;
}

static tacc_bool tacc_asm_file_iter_is_eof(struct tacc_asm_file_iter *iter) {
    return iter->src == iter->end;
}

static char tacc_asm_file_iter_peek_ch(struct tacc_asm_file_iter *iter) {
    char *cur;

    cur = iter->src;
    tacc_assert(
        ASSERT_DIAG, !tacc_asm_file_iter_is_eof(iter), "eof while peeking");
    return *cur;
}

static char tacc_asm_file_iter_consume_ch(struct tacc_asm_file_iter *iter) {
    char *cur;
    char ch;

    cur = iter->src;
    tacc_assert(
        ASSERT_DIAG, !tacc_asm_file_iter_is_eof(iter), "unexpected eof");
    ch = *cur;

    iter->src = iter->src + 1;
    return ch;
}

static tacc_bool tacc_asm_file_iter_accept_ch(struct tacc_asm_file_iter *iter,
                                              char accept) {
    if (tacc_asm_file_iter_is_eof(iter)) {
        return 0;
    }
    if (tacc_asm_file_iter_peek_ch(iter) == accept) {
        tacc_asm_file_iter_consume_ch(iter);
        return 1;
    }
    return 0;
}

static void tacc_asm_file_iter_eat_comment(struct tacc_asm_file_iter *iter) {
    char ch;

    tacc_assert(ASSERT_DIAG,
                tacc_asm_file_iter_accept_ch(iter, '/'),
                "error while scanning comment");
    tacc_assert(ASSERT_DIAG,
                tacc_asm_file_iter_accept_ch(iter, '*'),
                "error while scanning comment");

    while (1) {
        tacc_assert(
            ASSERT_DIAG, !tacc_asm_file_iter_is_eof(iter), "eof in comment");
        ch = tacc_asm_file_iter_consume_ch(iter);
        if (ch == '*') {
            if (tacc_asm_file_iter_accept_ch(iter, '/')) {
                break;
            }
        }
    }
}

static void
tacc_asm_file_iter_eat_line_comment(struct tacc_asm_file_iter *iter) {
    char ch;

    ch = tacc_asm_file_iter_peek_ch(iter);
    while (ch != '\n') {
        tacc_asm_file_iter_consume_ch(iter);
        ch = tacc_asm_file_iter_peek_ch(iter);
    }
}

static void
tacc_asm_file_iter_eat_ws_no_newlines(struct tacc_asm_file_iter *iter) {
    char ch;
    char *src;
    while (1) {
        if (tacc_asm_file_iter_accept_ch(iter, ' ')) {
            continue;
        }
        if (tacc_asm_file_iter_accept_ch(iter, 9)) {
            continue;
        }
        if (iter->target->at_comments) {
            if (tacc_asm_file_iter_accept_ch(iter, '@')) {
                tacc_asm_file_iter_eat_line_comment(iter);
                break;
            }
        }
        if (iter->target->hash_comments) {
            if (tacc_asm_file_iter_accept_ch(iter, '#')) {
                tacc_asm_file_iter_eat_line_comment(iter);
                break;
            }
        }
        if ((iter->end - iter->src) >= 2) {
            src = iter->src;
            ch = *src;
            if (ch == '/') {
                ch = src[1];
                if (ch == '*') {
                    tacc_asm_file_iter_eat_comment(iter);
                    continue;
                }
                if (ch == '/') {
                    tacc_asm_file_iter_eat_line_comment(iter);
                    continue;
                }
            }
        }
        break;
    }
}

tacc_bool tacc_is_labelic(char ch) {
    if (ch >= '0' && ch <= '9') {
        return 1;
    }
    if (ch >= 'a' && ch <= 'z') {
        return 1;
    }
    if (ch >= 'A' && ch <= 'Z') {
        return 1;
    }
    return ch == '_';
}

struct tacc_string *tacc_asm_try_parse_key(struct tacc_asm_file_iter *iter) {
    char *start;
    char *ch_ptr;
    char ch;
    struct tacc_string *result;

    start = iter->src;
    while (1) {
        ch_ptr = iter->src;
        ch = *ch_ptr;
        if (ch == '.' || ch == '$' || tacc_is_labelic(ch)) {
            iter->src = iter->src + 1;
            continue;
        }
        break;
    }
    if (start == iter->src) {
        return NULL;
    }
    result = tacc_dynstring_new();
    for (ch = 0; start < iter->src; start = start + 1) {
        tacc_dynstring_push(result, *start);
    }
    return result;
}

static struct tacc_string *tacc_asm_find_key(struct tacc_asm_file_iter *iter) {
    struct tacc_string *maybe_key;

    while (1) {
        tacc_asm_file_iter_eat_ws_no_newlines(iter);
        if (tacc_asm_file_iter_accept_ch(iter, '\n')) {
            return NULL;
        }

        maybe_key = tacc_asm_try_parse_key(iter);
        tacc_assert(
            ASSERT_DIAG, maybe_key, "invalid token to start an asm line");

        tacc_asm_file_iter_eat_ws_no_newlines(iter);
        if (tacc_asm_file_iter_accept_ch(iter, ':')) {
            tacc_assert(ASSERT_TODO, 0, "process labels");
        }

        return maybe_key;
    }
}

static struct tacc_asm_state *tacc_asm_state_new(void) {
    struct tacc_asm_state *state;

    state = tacc_malloc(sizeof(struct tacc_asm_state));
    state->out_file = NULL;

    return state;
}

static void tacc_asm_match_directive(struct tacc_asm_file_iter *iter,
                                     struct tacc_string *key,
                                     struct tacc_asm_state *state) {
    TACC_UNUSED(iter);
    TACC_UNUSED(key);
    TACC_UNUSED(state);

    tacc_assert(ASSERT_TODO, 0, "asm directives");
}

static tacc_bool tacc_asm_part_match(struct tacc_asm_statement *stmt,
                                     struct tacc_asm_match_action *action,
                                     struct tacc_asm_file_iter *iter,
                                     struct tacc_asm_state *state) {
    struct tacc_asm_statement_part *part;
    char *end;
    size_t len;

    switch (action->kind) {
    case ASMACT_STRTOK:
        tacc_asm_file_iter_eat_ws_no_newlines(iter);
        len = strlen(action->extra.str_tok);
        if (strncmp(iter->src, action->extra.str_tok, len)) {
            return 0;
        }

        end = iter->src + (len - 1);
        if (tacc_is_labelic(*end)) {
            end = iter->src + len;
            if (tacc_is_labelic(*end)) {
                /*
                 * the matched token is alphanumeric and its edge is not here.
                 * don't accept.
                 */
                return 0;
            }
        }

        iter->src = iter->src + len;
        tacc_asm_file_iter_eat_ws_no_newlines(iter);
        return 1;
    case ASMACT_MAYBE_STRTOK:
        tacc_asm_file_iter_eat_ws_no_newlines(iter);
        len = strlen(action->extra.str_tok);
        if (strncmp(iter->src, action->extra.str_tok, len)) {
            /* matcher accepts, even if not present */
            return 1;
        }

        end = iter->src + (len - 1);
        if (tacc_is_labelic(*end)) {
            end = iter->src + len;
            if (tacc_is_labelic(*end)) {
                /*
                 * the matched token is alphanumeric and its edge is not here.
                 * don't accept.
                 */
                return 0;
            }
        }

        iter->src = iter->src + len;
        tacc_asm_file_iter_eat_ws_no_newlines(iter);
        return 1;
    case ASMACT_EXPR:
        tacc_assert(ASSERT_TODO, 0, "asm: match expression");
        return 0;
    case ASMACT_PUSH_IMM32:
        part = tacc_asm_statement_part_new();
        part->kind = ASMPART_IMM32;
        part->extra.imm32 = action->extra.imm32;
        return 1;
    case ASMACT_PUSH_IMM64:
        part = tacc_asm_statement_part_new();
        part->kind = ASMPART_IMM32;
        part->extra.imm64 = action->extra.imm64;
        return 1;
    case ASMACT_EXPECT_LINESEP:
        tacc_asm_file_iter_eat_ws_no_newlines(iter);
        end = iter->src;
        if (*end != '\n' && *end != ';') {
            return 0;
        }
        return 1;

    default:
        return tacc_target_asm_part_match(stmt, action, iter, state);
    }
}

static tacc_bool tacc_asm_try_match(struct tacc_asm_candidate *candidate,
                                    struct tacc_asm_file_iter *iter,
                                    struct tacc_asm_state *state) {
    struct tacc_asm_match_action_list_entry *part_match;
    struct tacc_asm_statement *statement;
    size_t i;

    statement = tacc_asm_statement_new();
    for (i = 0; i < tacc_asm_match_action_list_len(candidate->actions);
         i = i + 1) {
        part_match = tacc_asm_match_action_list_get(candidate->actions, i);
        if (!tacc_asm_part_match(statement, part_match->content, iter, state)) {
            tacc_asm_statement_free(statement);
            return 0;
        }
    }
    tacc_target_asm_process_statement(statement, state);

    return 1;
}

static void tacc_asm_match(struct tacc_asm_file_iter *iter,
                           struct tacc_string *key,
                           struct tacc_asm_state *state) {
    struct tacc_asm_candidates_map_entry *candidates;
    struct tacc_asm_candidate_list_entry *candidate;
    char *saved_src;
    size_t i;

    if (tacc_dynstring_at(key, 0) == '.') {
        tacc_asm_match_directive(iter, key, state);
        return;
    }

    candidates = tacc_asm_candidates_map_get(state->matcher->matchers,
                                             tacc_dynstring_as_str(key));
    tacc_assert(ASSERT_DIAG,
                candidates != NULL,
                "no suitable matcher found for %s",
                tacc_dynstring_as_str(key));

    for (i = 0; i < tacc_asm_candidate_list_len(candidates->content->list);
         i = i + 1) {
        saved_src = iter->src;
        candidate = tacc_asm_candidate_list_get(candidates->content->list, i);
        if (tacc_asm_try_match(candidate->content, iter, state)) {
            return;
        }
        iter->src = saved_src;
    }
    tacc_assert(ASSERT_DIAG,
                0,
                "no suitable matcher found for %s",
                tacc_dynstring_as_str(key));
}

void taas_assemble(struct tacc_file *in_file, FILE *out_file) {
    struct tacc_string *key;
    struct tacc_asm_file_iter *iter;
    struct tacc_asm_state *state;

    iter = tacc_asm_file_iter_new_file(in_file, tacc_asm_target_new());
    state = tacc_asm_state_new();
    state->out_file = out_file;

    while (1) {
        if (iter->target->hash_comments_bol) {
            if (tacc_asm_file_iter_accept_ch(iter, '#')) {
                tacc_asm_file_iter_eat_line_comment(iter);
                continue;
            }
        }
        key = tacc_asm_find_key(iter);
        if (key != NULL) {
            tacc_asm_match(iter, key, state);
        } else {
            /* only labels, or perhaps an empty line */
        }
    }
}

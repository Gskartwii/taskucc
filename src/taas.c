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

struct tacc_asm_match_action *tacc_asm_match_action_new(void) {
    struct tacc_asm_match_action *action;

    action = tacc_malloc(sizeof(struct tacc_asm_match_action));
    action->kind = ASMACT_EXPR;

    return action;
}

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

struct tacc_asm_candidate *tacc_asm_candidate_new(void) {
    struct tacc_asm_candidate *cand;

    cand = tacc_malloc(sizeof(struct tacc_asm_candidate));
    cand->actions = tacc_asm_match_action_list_new();

    return cand;
}

struct tacc_asm_candidates *tacc_asm_candidates_new(void) {
    struct tacc_asm_candidates *candidates;

    candidates = tacc_malloc(sizeof(struct tacc_asm_candidates));
    candidates->list = tacc_asm_candidate_list_new();

    return candidates;
}

void tacc_asm_statement_part_free(struct tacc_asm_statement_part *part) {
    switch (part->kind) {
    case ASMPART_IMM32:
        break;
    case ASMPART_IMM64:
        tacc_free(part->extra.imm64);
        break;
    case ASMPART_SUB_ELEMENT:
        tacc_asm_statement_free(part->extra.sub_element);
        break;
    case ASMPART_TARGET:
        if (part->extra.target_data != NULL) {
            tacc_target_asm_statement_part_free(part->extra.target_data);
        }
        break;
    }
    tacc_free(part);
}

void tacc_asm_match_action_free(struct tacc_asm_match_action *action) {
    switch (action->kind) {
    case ASMACT_STRTOK:
        if (action->extra_owned) {
            tacc_free((void *) (action->extra.str_tok));
        }
        break;
    case ASMACT_SUB_FORM:
    case ASMACT_EXPR:
    case ASMACT_PUSH_IMM32:
    case ASMACT_EXPECT_LINESEP:
        break;
    case ASMACT_PUSH_IMM64:
        tacc_free(action->extra.imm64);
        break;
    default:
        if (action->extra.target_data != NULL) {
            tacc_target_asm_match_action_free(action->extra.target_data);
        }
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

void tacc_asm_file_iter_eat_ws_no_newlines(struct tacc_asm_file_iter *iter) {
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

static struct tacc_asm_matcher *tacc_asm_matcher_new(void) {
    struct tacc_asm_matcher *matcher;

    matcher = tacc_malloc(sizeof(struct tacc_asm_matcher));
    matcher->matchers = tacc_asm_candidates_map_new(0x1000);

    return matcher;
}

static struct tacc_asm_state *tacc_asm_state_new(void) {
    struct tacc_asm_state *state;

    state = tacc_malloc(sizeof(struct tacc_asm_state));
    state->out_file = NULL;
    state->matcher = tacc_asm_matcher_new();

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

tacc_bool tacc_asm_file_iter_accept_tok(struct tacc_asm_file_iter *iter,
                                        const char *tok) {
    size_t len;
    char *end;

    len = strlen(tok);
    if (strncmp(iter->src, tok, len)) {
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
    return 1;
}

tacc_bool tacc_asm_try_enum(struct tacc_asm_file_iter *iter,
                            const char **options,
                            size_t num_options,
                            uint32_t *chosen) {
    size_t i;
    char *pos;
    char **option;

    for (i = 0; i < num_options; i = i + 1) {
        pos = (char *) options;
        pos = pos + (sizeof(char *) * i);
        option = (char **) pos;
        if (tacc_asm_file_iter_accept_tok(iter, *option)) {
            *chosen = (uint32_t) i;
            return 1;
        }
    }

    return 0;
}

static struct tacc_asm_statement *
tacc_asm_match(struct tacc_asm_file_iter *iter,
               const char *key,
               struct tacc_asm_state *state);

tacc_bool tacc_asm_part_match(struct tacc_asm_statement *stmt,
                              struct tacc_asm_match_action *action,
                              struct tacc_asm_file_iter *iter,
                              struct tacc_asm_state *state) {
    struct tacc_asm_statement_part *part;
    char *end;

    switch (action->kind) {
    case ASMACT_STRTOK:
        tacc_asm_file_iter_eat_ws_no_newlines(iter);
        if (tacc_asm_file_iter_accept_tok(iter, action->extra.str_tok)) {
            tacc_asm_file_iter_eat_ws_no_newlines(iter);
            return 1;
        }
        return 0;
    case ASMACT_EXPR:
        tacc_assert(ASSERT_TODO, 0, "asm: match expression");
        return 0;
    case ASMACT_PUSH_IMM32:
        part = tacc_asm_statement_part_new();
        part->kind = ASMPART_IMM32;
        part->extra.imm32 = action->extra.imm32;
        break;
    case ASMACT_PUSH_IMM64:
        part = tacc_asm_statement_part_new();
        part->kind = ASMPART_IMM64;
        part->extra.imm64 = action->extra.imm64;
        break;
    case ASMACT_SUB_FORM:
        part = tacc_asm_statement_part_new();
        part->kind = ASMPART_SUB_ELEMENT;
        part->extra.sub_element =
            tacc_asm_match(iter, action->extra.str_tok, state);
        if (part->extra.sub_element == NULL) {
            tacc_free(part);
            return 0;
        }
        break;
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

    tacc_asm_statement_part_list_push(stmt->parts, part);
    return 1;
}

static struct tacc_asm_statement *
tacc_asm_try_match(struct tacc_asm_candidate *candidate,
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
            return NULL;
        }
    }

    return statement;
}

static struct tacc_asm_statement *
tacc_asm_match(struct tacc_asm_file_iter *iter,
               const char *key,
               struct tacc_asm_state *state) {
    struct tacc_asm_candidates_map_entry *candidates;
    struct tacc_asm_candidate_list_entry *candidate;
    struct tacc_asm_statement *statement;
    char *saved_src;
    size_t i;

    candidates = tacc_asm_candidates_map_get(state->matcher->matchers, key);
    tacc_assert(ASSERT_DIAG,
                candidates != NULL,
                "no suitable matcher found for %s",
                key);

    statement = NULL;
    for (i = 0; i < tacc_asm_candidate_list_len(candidates->content->list);
         i = i + 1) {
        saved_src = iter->src;
        candidate = tacc_asm_candidate_list_get(candidates->content->list, i);
        statement = tacc_asm_try_match(candidate->content, iter, state);
        if (statement != NULL) {
            break;
        }
        iter->src = saved_src;
    }
    return statement;
}

void tacc_asm_form(struct tacc_asm_matcher *matcher,
                   const char *key,
                   const char *template,
                   ...) {
    va_list parts;

    va_start(parts, template);
    tacc_asm_vform(matcher, key, template, parts);
    va_end(parts);
}

static const char *tacc_asm_strtok_from_template(const char *template,
                                                 size_t *i) {
    const char *start;
    const char *end;
    char *tok;
    char *tok_end;
    char start_char;
    size_t tok_length;

    start = template + (*i);
    start_char = *start;
    if (tacc_is_labelic(start_char)) {
        end = strchr(start, ' ');
        tok_length = (size_t) (end - start);
    } else {
        tok_length = 1;
    }

    tok = tacc_malloc(sizeof(char) * (tok_length + 1));
    strncpy(tok, start, tok_length);
    *i = (*i) + tok_length;
    tok_end = tok + tok_length;
    *tok_end = 0;

    return tok;
}

void tacc_asm_vform(struct tacc_asm_matcher *matcher,
                    const char *key,
                    const char *template,
                    va_list parts) {
    struct tacc_asm_candidate *cand;
    struct tacc_asm_match_action *action;
    struct tacc_asm_candidates_map_entry *map_entry;
    struct tacc_asm_candidates *candidates;
    const char *template_pos;
    const char *in_str;
    uint32_t in_imm32;
    char template_ch;
    tacc_bool expect_linesep;
    size_t i;

    cand = tacc_asm_candidate_new();

    /*
     * '@' in key signals that this is a partial form and not an actual
     * instruction
     */
    expect_linesep = (*key) != '@';
    for (i = 0; i < strlen(template); i = i + 1) {
        template_pos = template + i;
        template_ch = *template_pos;
        if (template_ch == ' ') {
            continue;
        }
        if (template_ch == '%') {
            i = i + 1;
            template_pos = template + i;
            template_ch = *template_pos;
            if (template_ch == '?') {
                in_str = va_arg(parts, void *);
                action = tacc_asm_match_action_new();
                action->kind = ASMACT_SUB_FORM;
                action->extra.str_tok = in_str;
                action->extra.str_tok = in_str;
                tacc_asm_match_action_list_push(cand->actions, action);
                continue;
            }
            if (template_ch == 'd') {
                in_imm32 = va_arg(parts, uint32_t);
                action = tacc_asm_match_action_new();
                action->kind = ASMACT_PUSH_IMM32;
                action->extra.imm32 = in_imm32;
                tacc_asm_match_action_list_push(cand->actions, action);
                continue;
            }
            if (template_ch == 's') {
                in_str = va_arg(parts, void *);
                action = tacc_asm_match_action_new();
                action->kind = ASMACT_STRTOK;
                action->extra.str_tok = in_str;
                action->extra_owned = 0;
                tacc_asm_match_action_list_push(cand->actions, action);
                continue;
            }
            if (template_ch == '$') {
                expect_linesep = 0;
                continue;
            }
            tacc_assert(ASSERT_ICE,
                        template_ch == '%',
                        "bad character in tacc_asm_vform()");
        }
        action = tacc_asm_match_action_new();
        action->kind = ASMACT_STRTOK;
        action->extra.str_tok = tacc_asm_strtok_from_template(template, &i);
        action->extra_owned = 1;
        tacc_asm_match_action_list_push(cand->actions, action);
    }
    if (expect_linesep) {
        action = tacc_asm_match_action_new();
        action->kind = ASMACT_EXPECT_LINESEP;
        tacc_asm_match_action_list_push(cand->actions, action);
    }

    map_entry = tacc_asm_candidates_map_get(matcher->matchers, key);
    if (map_entry != NULL) {
        candidates = map_entry->content;
    } else {
        candidates = tacc_asm_candidates_new();
        candidates->key = key;
        tacc_asm_candidates_map_insert(matcher->matchers, candidates);
    }
    tacc_asm_candidate_list_push(candidates->list, cand);
}

uint32_t tacc_asm_stmt_get_i32(struct tacc_asm_statement *stmt, size_t index) {
    struct tacc_asm_statement_part_list_entry *entry;

    entry = tacc_asm_statement_part_list_get(stmt->parts, index);
    tacc_assert(ASSERT_ICE,
                entry->content->kind == ASMPART_IMM32,
                "didn't get imm32 at %d",
                index);

    return entry->content->extra.imm32;
}

struct tacc_asm_statement *
tacc_asm_stmt_get_sub(struct tacc_asm_statement *stmt, size_t index) {
    struct tacc_asm_statement_part_list_entry *entry;

    entry = tacc_asm_statement_part_list_get(stmt->parts, index);
    tacc_assert(ASSERT_ICE,
                entry->content->kind == ASMPART_SUB_ELEMENT,
                "didn't get subelement at %d",
                index);

    return entry->content->extra.sub_element;
}

struct tacc_asm_match_action *tacc_asm_target_matcher(
    uint32_t kind, struct tacc_target_asm_match_action *target_data) {
    struct tacc_asm_match_action *action;

    action = tacc_asm_match_action_new();
    action->kind = kind;
    action->extra.target_data = target_data;

    return action;
}

static void tacc_asm_matcher_free(struct tacc_asm_matcher *matcher) {
    tacc_asm_candidates_map_free(matcher->matchers);
    tacc_free(matcher->matchers);
    tacc_free(matcher);
}

static void tacc_asm_state_free(struct tacc_asm_state *state) {
    tacc_asm_matcher_free(state->matcher);
    tacc_free(state);
}

void tacc_asm_out8(struct tacc_asm_state *state, uint8_t data) {
    if ((data & 0xf) == data) {
        fprintf(state->out_file, "0%x ", data);
    } else {
        fprintf(state->out_file, "%x ", data);
    }
}

void taas_assemble(struct tacc_file *in_file, FILE *out_file) {
    struct tacc_string *key;
    struct tacc_asm_target *target;
    struct tacc_asm_file_iter *iter;
    struct tacc_asm_state *state;
    struct tacc_asm_statement *statement;

    target = tacc_asm_target_new();
    iter = tacc_asm_file_iter_new_file(in_file, target);
    tacc_free(in_file);
    state = tacc_asm_state_new();
    tacc_target_asm_init_matchers(state->matcher);
    state->out_file = out_file;

    while (1) {
        if (tacc_asm_file_iter_is_eof(iter)) {
            break;
        }
        if (iter->target->hash_comments_bol) {
            if (tacc_asm_file_iter_accept_ch(iter, '#')) {
                tacc_asm_file_iter_eat_line_comment(iter);
                continue;
            }
        }
        key = tacc_asm_find_key(iter);
        if (key != NULL) {
            if (tacc_dynstring_at(key, 0) == '.') {
                tacc_asm_match_directive(iter, key, state);
                continue;
            }

            statement = tacc_asm_match(iter, tacc_dynstring_as_str(key), state);
            tacc_assert(ASSERT_DIAG,
                        statement != NULL,
                        "no matcher found for %s",
                        tacc_dynstring_as_str(key));
            tacc_target_asm_process_statement(statement, state);
            tacc_asm_statement_free(statement);

            tacc_dynstring_free(key);
            key = NULL;
        } else {
            /* only labels, or perhaps an empty line */
        }
    }

    tacc_free(target);
    tacc_asm_file_iter_free(iter);
    tacc_asm_state_free(state);
}

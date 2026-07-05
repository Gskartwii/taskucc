#include "dynstring.h"
#include "gcc_compat.h"
#include "target_defs.h"
#include "tasku_file.h"
#include "tasku_pp.h"
#include "test.h"
#include "util.h"
#include <string.h>

struct tacc_options {
    char *filename;

    struct tacc_string_list *defines;
    tacc_bool preprocess;
};

#define ARG_SHORT_APPEND(ch, to_list)                                         \
    do {                                                                      \
        if (*arg == ch) {                                                     \
            arg = arg + 1;                                                    \
            if (!*arg) {                                                      \
                i = i + 1;                                                    \
                tacc_assert(i < count, "incomplete argument -%s\n", arg - 1); \
                arg = argv[i];                                                \
            }                                                                 \
            str = tacc_dynstring_new();                                       \
            tacc_dynstring_concat(str, arg);                                  \
            tacc_string_list_push(to_list, str);                              \
            goto next;                                                        \
        }                                                                     \
    } while (0)
#define ARG_SHORT_SET_FLAG(ch, flag)                                          \
    do {                                                                      \
        if (*arg == ch) {                                                     \
            arg = arg + 1;                                                    \
            tacc_assert(                                                      \
                (*arg) == 0, "cannot take content for flag: -%s\n", arg - 1); \
            flag = 1;                                                         \
            goto next;                                                        \
        }                                                                     \
    } while (0)

static void tacc_parse_options(struct tacc_options *options,
                               int argc,
                               char **argv) {
    size_t i;
    size_t count;
    char *arg;
    struct tacc_string *str;

    options->filename = NULL;
    options->defines = tacc_string_list_new();
    options->preprocess = 0;

    count = (size_t) argc;
    for (i = 1; i < count; i = i + 1) {
        arg = argv[i];
        if (*arg != '-') {
            tacc_assert(!options->filename, "multiple filenames given");
            options->filename = arg;
            continue;
        }
        arg = arg + 1;
        ARG_SHORT_APPEND('D', options->defines);
        ARG_SHORT_SET_FLAG('E', options->preprocess);
        tacc_assert(0, "invalid option %s\n", argv[i]);
#ifdef __M2__
    next:
        0;
#else
    next:;
#endif
    }
}

tacc_bool tacc_print_trivia(char *trivia_content) {
    char *line_start;
    size_t i;
    tacc_bool printed_indent;

    line_start = strrchr(trivia_content, '\n');
    printed_indent = 0;
    if (line_start == NULL) {
        printf(" ");
    } else {
        printf("\n");
        for (i = 0; i < strlen(trivia_content) -
                            (size_t) (line_start - trivia_content) - 1;
             i = i + 1) {
            printf(" ");
            printed_indent = 1;
        }
    }
    return printed_indent;
}

void tacc_output_pp(struct tacc_tok_iter *tok_iter) {
    struct pp_tok *token;
    struct pp_tok *pending_trivia;
    enum pp_tok_kind last_tok_kind;
    char *content;
    tacc_bool ws_acked;

    pending_trivia = NULL;
    last_tok_kind = TOK_UNRECOGNIZED;
    while (1) {
        ws_acked = 0;
        token = tacc_tok_iter_next(tok_iter);
        if (token->kind == TOK_EOF) {
            tacc_pp_tok_free(token);
            break;
        }
        if (token->kind == TOK_FAKE_TRIVIA) {
            if (pending_trivia) {
                tacc_pp_tok_free(pending_trivia);
            }
            pending_trivia = token;
        } else {
            if (pending_trivia) {
                if (!tacc_print_trivia(pending_trivia->str->string) &&
                    token->preceded_by_ws) {
                    printf(" ");
                    ws_acked = 1;
                }
                tacc_pp_tok_free(pending_trivia);
                pending_trivia = NULL;
            } else if (token->preceded_by_ws) {
                printf(" ");
                ws_acked = 1;
            }

            content = tacc_pp_tok_content(token);
            if (!ws_acked) {
                if (last_tok_kind == TOK_PPNUM) {
                    if ((*content) >= '0' && (*content) <= '9') {
                        printf(" ");
                    }
                    if ((*content) == '.' || (*content) == '-') {
                        printf(" ");
                    }
                }
            }

            printf("%s", content);
            last_tok_kind = token->kind;
            tacc_pp_tok_free(token);
        }
    }
    puts("");
}

void tacc_apply_defines(struct tacc_string_list *defines,
                        struct tacc_pp_state *state) {
    struct tacc_string_list_entry *str_entry;
    struct tacc_string *str;
    size_t i;
    char *define_str;
    char *macro_str;
    char *val_str;

    for (i = 0; i < tacc_string_list_len(defines); i = i + 1) {
        str_entry = tacc_string_list_get(defines, i);
        str = str_entry->content;
        define_str = tacc_dynstring_as_str(str);
        val_str = strchr(define_str, '=');
        if (val_str) {
            macro_str = tacc_malloc(strlen(define_str) + 1);
            strcpy(macro_str, define_str);
            define_str = macro_str + (val_str - define_str);
            *define_str = 0;
            val_str = val_str + 1;
        } else {
            val_str = "1";
            macro_str = tacc_malloc(strlen(define_str) + 1);
            strcpy(macro_str, define_str);
        }

        tacc_pp_define(state, macro_str, val_str);
        tacc_free(macro_str);
    }
}

int main(int argc, char **argv) {
    struct tacc_file *input_file;
    struct tacc_file_iter *file_iter;
    struct tacc_tok_iter *tok_iter;
    struct tacc_pp_state *pp_state;
    struct tacc_target *target;
    struct tacc_options options;

    init_io();

    tacc_parse_options(&options, argc, argv);
    tacc_assert(options.filename != NULL, "need filename");

    if (!strcmp(options.filename, "run-tests")) {
        tacc_string_list_free(options.defines);
        tacc_free(options.defines);
        return run_tests();
    }

    target = tacc_target_new("x86_64-linux");
    pp_state = tacc_pp_state_new(target);
    tacc_apply_defines(options.defines, pp_state);
    tacc_string_list_free(options.defines);
    tacc_free(options.defines);

    input_file = tacc_open(options.filename);
    file_iter = tacc_file_iter_new_file(input_file);
    tok_iter = tacc_tok_iter_new(file_iter, pp_state);

    tacc_assert(options.preprocess, "unsupported mode: not -E");
    tacc_output_pp(tok_iter);

    tacc_tok_iter_free(tok_iter);
    tacc_target_free(target);
    tacc_pp_state_free(pp_state);

    return 0;
}

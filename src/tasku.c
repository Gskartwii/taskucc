#include "dynstring.h"
#include "gcc_compat.h"
#include "target_defs.h"
#include "tasku_file.h"
#include "tasku_pp.h"
#include "test.h"
#include "util.h"
#include <string.h>

struct tacc_options {
    struct tacc_string_list *defines;
    char *filename;
};

static void tacc_parse_options(struct tacc_options *options,
                               int argc,
                               char **argv) {
    size_t i;
    size_t count;
    char *arg;
    struct tacc_string *str;

    options->filename = NULL;
    options->defines = tacc_string_list_new();

    count = (size_t) argc;
    for (i = 1; i < count; i = i + 1) {
        arg = argv[i];
        if (*arg != '-') {
            tacc_assert(!options->filename, "multiple filenames given");
            options->filename = arg;
            continue;
        }
        arg = arg + 1;
        if (*arg == 'D') {
            arg = arg + 1;
            if (!*arg) {
                i = i + 1;
                arg = argv[i];
            }
            str = tacc_dynstring_new();
            tacc_dynstring_concat(str, arg);
            tacc_string_list_push(options->defines, str);
            if (!strchr(arg, '=')) {
                tacc_dynstring_concat(str, "=1");
            }
        }
    }
}

int main(int argc, char **argv) {
    size_t i;
    struct tacc_file *input_file;
    struct tacc_file_iter *file_iter;
    struct tacc_tok_iter *tok_iter;
    struct tacc_pp_state *pp_state;
    struct tacc_target *target;
    struct pp_tok *token;
    struct tacc_options options;
    struct tacc_string_list_entry *str_entry;
    struct tacc_string *str;
    char *define_str;
    char *macro_str;
    char *val_str;

    init_io();

    tacc_parse_options(&options, argc, argv);

    tacc_assert(options.filename != NULL, "need filename");

    target = tacc_target_new("x86_64-linux");
    pp_state = tacc_pp_state_new(target);
    for (i = 0; i < tacc_string_list_len(options.defines); i = i + 1) {
        str_entry = tacc_string_list_get(options.defines, i);
        str = str_entry->content;
        define_str = tacc_dynstring_as_str(str);
        val_str = strchr(define_str, '=');
        macro_str = tacc_malloc(strlen(define_str) + 1);
        strcpy(macro_str, define_str);
        define_str = macro_str + (val_str - define_str);
        *define_str = 0;
        val_str = val_str + 1;

        tacc_pp_define(pp_state, macro_str, define_str);
        tacc_free(macro_str);
    }
    tacc_string_list_free(options.defines);
    tacc_free(options.defines);

    if (!strcmp(options.filename, "run-tests")) {
        tacc_target_free(target);
        tacc_pp_state_free(pp_state);
        return run_tests();
    }

    input_file = tacc_open(options.filename);
    file_iter = tacc_file_iter_new_file(input_file);
    input_file = NULL;
    tok_iter = tacc_tok_iter_new(file_iter, pp_state);
    file_iter = NULL;

    while (1) {
        token = tacc_tok_iter_next(tok_iter);
        if (token->kind == TOK_EOF) {
            tacc_pp_tok_free(token);
            break;
        }
        printf("%s: %s\n",
               tacc_pp_to_string(token),
               tacc_dynstring_as_str(token->str));
        tacc_pp_tok_free(token);
    }

    tacc_tok_iter_free(tok_iter);
    tacc_target_free(target);
    tacc_pp_state_free(pp_state);

    return 0;
}

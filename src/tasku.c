#include "gcc_compat.h"
#include "target_defs.h"
#include "tasku_file.h"
#include "tasku_pp.h"
#include "test.h"
#include "util.h"
#include <string.h>

int main(int argc, char **argv) {
    char *filename;
    struct tacc_file *input_file;
    struct tacc_file_iter *file_iter;
    struct tacc_tok_iter *tok_iter;
    struct tacc_pp_state *pp_state;
    struct tacc_target *target;
    struct pp_tok *token;

    init_io();

    tacc_assert(argc >= 2, "need filename");
    filename = argv[1];

    if (!strcmp(filename, "run-tests")) {
        return run_tests();
    }

    input_file = tacc_open(filename);

    target = tacc_target_new("x86_64-linux");
    file_iter = tacc_file_iter_new_file(input_file);
    input_file = NULL;
    pp_state = tacc_pp_state_new(target);
    tok_iter = tacc_tok_iter_new(file_iter, pp_state);
    file_iter = NULL;

    while (1) {
        token = tacc_tok_iter_next(tok_iter);
        if (token->kind == TOK_EOF) {
            break;
        }
        printf("%s: %s\n",
               tacc_pp_to_string(token),
               tacc_dynstring_as_str(token->str));
        tacc_pp_tok_free(token);
    }

    tacc_tok_iter_free(tok_iter);
    tacc_pp_state_free(pp_state);
    tacc_target_free(target);

    return 0;
}

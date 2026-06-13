#include "gcc_compat.h"
#include "tasku_file.h"
#include "tasku_pp.h"
#include "util.h"

int main(int argc, char **argv) {
    char *filename;
    tacc_file_p input_file;
    tacc_file_iter_p file_iter;
    tacc_tok_iter_p tok_iter;
    pp_tok_p token;

    init_io();

    tacc_assert(argc >= 2, "need filename");
    filename = argv[1];

    input_file = tacc_open(filename);

    file_iter = tacc_file_iter_new();
    tacc_file_iter_init(file_iter, input_file);
    tok_iter = tacc_tok_iter_new();
    tacc_tok_iter_init(tok_iter, file_iter);

    while (1) {
        token = tacc_tok_iter_next(tok_iter);
        if (token->pp_tok__kind == TOK_EOF) {
            break;
        }
        printf("%s: %s\n", tacc_pp_to_string(token), token->pp_tok_str);
    }

    return 0;
}

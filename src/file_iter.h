#ifndef TACC_FILE_ITER_H
#define TACC_FILE_ITER_H

#include "tasku_file.h"
#include "util.h"

struct tacc_file_iter {
    /* owning */
    char *orig;
    /* borrow of orig */
    char *src;
    /* borrow of src */
    char *end;
    /* owning, possibly null */
    char *filename;
    tacc_bool is_bol;
    tacc_bool is_ws;
    tacc_bool permit_splice;
    tacc_bool at_comments;
    tacc_bool hash_comments;
    tacc_bool hash_comments_bol;
    tacc_bool slash_2_comments;
};

/* return: owning */
struct tacc_file_iter *tacc_file_iter_new_file(struct tacc_file *file);
struct tacc_file_iter *tacc_file_iter_new_str(char *start, char *end);
char tacc_file_iter_consume_ch(struct tacc_file_iter *iter);
tacc_bool tacc_file_iter_accept_ch(struct tacc_file_iter *iter, char accept);
char tacc_file_iter_peek_ch(struct tacc_file_iter *iter);
tacc_bool tacc_file_iter_is_eof(struct tacc_file_iter *iter);
/* iter: owning */
void tacc_file_iter_free(struct tacc_file_iter *iter);
void tacc_file_iter_eat_ws_no_newlines(struct tacc_file_iter *iter);

#endif

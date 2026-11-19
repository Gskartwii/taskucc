#include "file_iter.h"

void tacc_file_iter_free(struct tacc_file_iter *iter) {
    if (iter->filename) {
        tacc_free(iter->filename);
    }
    tacc_free(iter->orig);
    tacc_free(iter);
}

/* iter: borrow, start: owning, end: owning */
static void tacc_file_iter_init_str(struct tacc_file_iter *iter,
                                    char *start,
                                    char *end) {
    iter->is_bol = 1;
    iter->is_ws = 1;
    iter->orig = start;
    iter->src = start;
    iter->end = end;
    iter->filename = NULL;
    iter->slash_2_comments = 1;
    iter->at_comments = 0;
    iter->hash_comments = 0;
    iter->permit_splice = 1;
}

/* iter: borrow, file: owning */
void tacc_file_iter_init(struct tacc_file_iter *iter, struct tacc_file *file) {
    tacc_file_iter_init_str(iter, file->src, file->src + file->len);
    iter->filename = file->name;
    tacc_free(file);
}

/* return: owning */
struct tacc_file_iter *tacc_file_iter_new_file(struct tacc_file *file) {
    struct tacc_file_iter *iter;

    iter = tacc_malloc(sizeof(struct tacc_file_iter));
    tacc_file_iter_init(iter, file);

    return iter;
}

/* return: owning */
struct tacc_file_iter *tacc_file_iter_new_str(char *start, char *end) {
    struct tacc_file_iter *iter;

    iter = tacc_malloc(sizeof(struct tacc_file_iter));
    tacc_file_iter_init_str(iter, start, end);

    return iter;
}

/* iter: borrow */
static void tacc_file_iter_eat_splices(struct tacc_file_iter *iter) {
    char ch;
    char *src;

    src = iter->src;
    ch = *src;

    if (ch == '\\') {
        /* c99 5.1.1.2p2: a physical source file cannot end with a backslash */
        ch = iter->src[1];
        if (ch == '\n') {
            iter->src = iter->src + 1;
            iter->src = iter->src + 1;
        }
    }
}

/* return: borrow, iter: borrow */
static char *tacc_file_iter_cur(struct tacc_file_iter *iter) {
    if (iter->permit_splice) {
        tacc_file_iter_eat_splices(iter);
    }
    return iter->src;
}

/* iter: borrow */
tacc_bool tacc_file_iter_is_eof(struct tacc_file_iter *iter) {
    if (tacc_file_iter_cur(iter) == iter->end) {
        return 1;
    }
    return 0;
}

/* iter: borrow */
char tacc_file_iter_peek_ch(struct tacc_file_iter *iter) {
    char *cur;

    cur = tacc_file_iter_cur(iter);
    tacc_assert(ASSERT_DIAG, !tacc_file_iter_is_eof(iter), "eof while peeking");
    return *cur;
}

/* iter: borrow */
char tacc_file_iter_consume_ch(struct tacc_file_iter *iter) {
    char *cur;
    char ch;

    cur = tacc_file_iter_cur(iter);
    tacc_assert(ASSERT_DIAG, !tacc_file_iter_is_eof(iter), "unexpected eof");
    ch = *cur;

    /*
     * newline -> set bol flag, unset ws flag
     * whitespace -> keep bol flag, set ws flag
     * otherwise -> unset both
     */
    if (ch == '\n') {
        iter->is_bol = 1;
        iter->is_ws = 0;
    } else if (ch == 9 || ch == 12 || ch == ' ') {
        iter->is_ws = 1;
    } else {
        iter->is_ws = 0;
        iter->is_bol = 0;
    }

    iter->src = iter->src + 1;
    return ch;
}

/* iter: borrow */
tacc_bool tacc_file_iter_accept_ch(struct tacc_file_iter *iter, char accept) {
    if (tacc_file_iter_is_eof(iter)) {
        return 0;
    }
    if (tacc_file_iter_peek_ch(iter) == accept) {
        tacc_file_iter_consume_ch(iter);
        return 1;
    }
    return 0;
}

/* iter: borrow */
static void tacc_file_iter_eat_comment(struct tacc_file_iter *iter) {
    char ch;

    tacc_assert(ASSERT_DIAG,
                tacc_file_iter_accept_ch(iter, '/'),
                "error while scanning comment");
    tacc_assert(ASSERT_DIAG,
                tacc_file_iter_accept_ch(iter, '*'),
                "error while scanning comment");

    while (1) {
        tacc_assert(
            ASSERT_DIAG, !tacc_file_iter_is_eof(iter), "eof in comment");
        ch = tacc_file_iter_consume_ch(iter);
        if (ch == '*') {
            if (tacc_file_iter_accept_ch(iter, '/')) {
                break;
            }
        }
    }
    /* per standard, replaced by single space */
    iter->is_ws = 1;
}

/* iter: borrow */
static void tacc_file_iter_eat_line_comment(struct tacc_file_iter *iter) {
    char ch;

    ch = tacc_file_iter_peek_ch(iter);
    while (ch != '\n') {
        tacc_file_iter_consume_ch(iter);
        ch = tacc_file_iter_peek_ch(iter);
    }
}

/* iter: borrow */
void tacc_file_iter_eat_ws_no_newlines(struct tacc_file_iter *iter) {
    char ch;
    char *src;
    while (1) {
        if (tacc_file_iter_accept_ch(iter, ' ')) {
            continue;
        }
        if (tacc_file_iter_accept_ch(iter, 9)) {
            continue;
        }
        if (iter->at_comments) {
            if (tacc_file_iter_accept_ch(iter, '@')) {
                tacc_file_iter_eat_line_comment(iter);
                break;
            }
        }
        if (iter->hash_comments) {
            if (tacc_file_iter_accept_ch(iter, '#')) {
                tacc_file_iter_eat_line_comment(iter);
                break;
            }
        }
        if ((iter->end - iter->src) >= 2) {
            src = iter->src;
            ch = *src;
            if (ch == '/') {
                ch = src[1];
                if (ch == '*') {
                    tacc_file_iter_eat_comment(iter);
                    continue;
                }
                if (ch == '/') {
                    tacc_file_iter_eat_line_comment(iter);
                    continue;
                }
            }
        }
        break;
    }
}

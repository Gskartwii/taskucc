#ifndef TACC_PARSE_H
#define TACC_PARSE_H

#include "expr.h"
#include "tasku_pp.h"

struct tacc_expr *tacc_parse_new_expr(struct tacc_tok_iter *iter);

struct tacc_ast {
    struct tacc_decl_list *declarations;
};

#endif

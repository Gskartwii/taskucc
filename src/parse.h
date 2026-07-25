#ifndef TACC_PARSE_H
#define TACC_PARSE_H

#include "expr.h"
#include "tasku_pp.h"
#include "type.h"

struct tacc_expr *tacc_parse_new_expr(struct tacc_tok_iter *iter);
struct tacc_ast *tacc_parse_file(struct tacc_type_registry *registry,
                                 struct tacc_tok_iter *iter);
void tacc_ast_free(struct tacc_ast *ast);

struct tacc_ast {
    struct tacc_decl_list *declarations;
};

#endif

#ifndef TACC_PARSE_H
#define TACC_PARSE_H

#include "expr.h"
#include "tasku_pp.h"
#include "type.h"

enum tacc_untagged_ident_kind { UNTAGGED_IDENT_OBJECT, UNTAGGED_IDENT_TYPEDEF };

struct tacc_untagged_ident {
    struct tacc_string *name;
    enum tacc_untagged_ident_kind kind;
    struct tacc_type *base_type;
};

DECL_DYNARRAY_OVER(tacc_untagged_ident_list,
                   tacc_untagged_ident_list_entry,
                   struct tacc_untagged_ident *,
                   tacc_untagged_ident_list_new,
                   tacc_untagged_ident_list_init,
                   tacc_untagged_ident_list_get,
                   tacc_untagged_ident_list_push,
                   tacc_untagged_ident_list_pop,
                   tacc_untagged_ident_list_len,
                   tacc_untagged_ident_list_free)

DECL_DYNARRAY_OVER(tacc_ident_scope_list,
                   tacc_ident_scope_list_entry,
                   struct tacc_ident_scope *,
                   tacc_ident_scope_list_new,
                   tacc_ident_scope_list_init,
                   tacc_ident_scope_list_get,
                   tacc_ident_scope_list_push,
                   tacc_ident_scope_list_pop,
                   tacc_ident_scope_list_len,
                   tacc_ident_scope_list_free)

struct tacc_ident_scope {
    struct tacc_untagged_ident_list *untagged_idents;
    struct tacc_compound_type_list *tagged_types;
};

void tacc_ident_scope_free(struct tacc_ident_scope *scope);

struct tacc_parse_registry {
    struct tacc_target *target;
    struct tacc_type_list *basic_types;

    struct tacc_ident_scope_list *scopes;
};

struct tacc_parse_registry *tacc_type_registry_new(struct tacc_target *target);
void tacc_type_registry_free(struct tacc_parse_registry *registry);
struct tacc_expr *tacc_parse_new_expr(struct tacc_tok_iter *iter);
struct tacc_ast *tacc_parse_file(struct tacc_parse_registry *registry,
                                 struct tacc_tok_iter *iter);
void tacc_untagged_ident_free(struct tacc_untagged_ident *ident);
void tacc_ast_free(struct tacc_ast *ast);

struct tacc_ast {
    struct tacc_decl_list *declarations;
};

#endif

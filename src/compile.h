#ifndef TACC_COMPILE_H
#define TACC_COMPILE_H

#include "decl.h"
#include "dynarray.h"
#include "dynhash.h"
#include "target/target.h"

enum tacc_compile_ident_kind {
    COMPIDENT_TYPEDEF,
    COMPIDENT_GLOBAL_OBJ,
    COMPIDENT_LOCAL_VAR,
    COMPIDENT_ENUMERATOR
};

struct tacc_compile_ident {
    enum tacc_compile_ident_kind kind;
    struct tacc_string *ident;
};

struct tacc_block_scope {
    struct tacc_compile_ident_map *untagged_idents;
    struct tacc_type_map *tagged_types;
};

DECL_DYNARRAY_OVER(tacc_block_scope_list,
                   tacc_block_scope_list_entry,
                   struct tacc_block_scope *,
                   tacc_block_scope_list_new,
                   tacc_block_scope_list_init,
                   tacc_block_scope_list_get,
                   tacc_block_scope_list_push,
                   tacc_block_scope_list_pop,
                   tacc_block_scope_list_len,
                   tacc_block_scope_list_free)

DECL_DYNHASH_OVER(tacc_compile_ident_map,
                  tacc_compile_ident_map_entry,
                  struct tacc_compile_ident *,
                  tacc_compile_ident_map_new,
                  tacc_compile_ident_map_init,
                  tacc_compile_ident_map_get,
                  tacc_compile_ident_map_push,
                  tacc_compile_ident_map_count,
                  tacc_compile_ident_map_free)

struct tacc_compiler {
    struct tacc_target *target;
    struct tacc_type_list *basic_types;
    struct tacc_type_list *anonymous_types;

    struct tacc_block_scope_list *block_scopes;
};

struct tacc_block_scope *tacc_block_scope_new(void);
void tacc_block_scope_free(struct tacc_block_scope *scope);
void tacc_compile_ident_free(struct tacc_compile_ident *ident);
void tacc_compile_prelude(struct tacc_compiler *compiler);
struct tacc_type *tacc_type_from_decl_type(struct tacc_compiler *compiler,
                                           struct tacc_decl_type *type);
void tacc_compile_top_decl(struct tacc_compiler *compiler,
                           struct tacc_decl *decl);
void tacc_compile_output_directive(struct tacc_compiler *compiler,
                                   char *directive_fmt,
                                   ...);
void tacc_compile_output(struct tacc_compiler *compiler, char *fmt, ...);

#endif

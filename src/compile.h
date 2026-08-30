#ifndef TACC_COMPILE_H
#define TACC_COMPILE_H

#include "decl.h"
#include "dynarray.h"
#include "target/target.h"

struct tacc_compiler {
    struct tacc_target *target;
    struct tacc_type_list *basic_types;
    struct tacc_type_list *anonymous_types;
    struct tacc_type_list *named_types;

    /* borrow */
    struct tacc_string_list *names;
};

DECL_DYNARRAY_OVER(tacc_ident_list,
                   tacc_ident_list_entry,
                   uint32_t,
                   tacc_ident_list_new,
                   tacc_ident_list_init,
                   tacc_ident_list_get,
                   tacc_ident_list_push,
                   tacc_ident_list_pop,
                   tacc_ident_list_len,
                   tacc_ident_list_free)

struct tacc_block_scope *tacc_block_scope_new(void);
void tacc_block_scope_free(struct tacc_block_scope *scope);
void tacc_compile_prelude(struct tacc_compiler *compiler);
struct tacc_type *tacc_type_from_decl_type(struct tacc_compiler *compiler,
                                           struct tacc_decl_type *type);
void tacc_compile_top_decl(struct tacc_compiler *compiler,
                           struct tacc_decl *decl);
void tacc_compile_output_directive(struct tacc_compiler *compiler,
                                   char *directive_fmt,
                                   ...);
struct tacc_string *tacc_compile_get_name(struct tacc_compiler *compiler,
                                          uint32_t name_ref);
void tacc_compile_output(struct tacc_compiler *compiler, char *fmt, ...);

#endif

#ifndef TACC_COMPILE_H
#define TACC_COMPILE_H

#include "decl.h"
#include "dynarray.h"
#include "dynhash.h"
#include "target/target.h"

struct tacc_global_object {
    uint32_t name_ref;

    tacc_bool is_enumerator;
    union {
        struct tacc_val *enumerator_value;
        struct tacc_type *obj_type;
    } extra;
};

DECL_DYNHASH_OVER_U32(tacc_global_object_map,
                      tacc_global_object_map_entry,
                      struct tacc_global_object *,
                      tacc_global_object_map_new,
                      tacc_global_object_map_init,
                      tacc_global_object_map_get,
                      tacc_global_object_map_insert,
                      tacc_global_object_map_fill_count,
                      tacc_global_object_map_free)

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

struct tacc_compiler {
    struct tacc_target *target;
    struct tacc_type_list *basic_types;
    struct tacc_type_list *anonymous_types;
    struct tacc_type_list *named_types;
    struct tacc_global_object_map *global_objects;

    /* borrow */
    struct tacc_string_list *names;
};

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
struct tacc_type *
tacc_type_adjust_from_declarator(struct tacc_compiler *compiler,
                                 struct tacc_type *base_type,
                                 struct tacc_declarator *declarator);
struct tacc_string *tacc_compile_get_name(struct tacc_compiler *compiler,
                                          uint32_t name_ref);
void tacc_compile_output(struct tacc_compiler *compiler, char *fmt, ...);
struct tacc_global_object *
tacc_compile_resolve_global(struct tacc_compiler *compiler, uint32_t name_ref);

#endif

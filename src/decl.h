#ifndef TACC_DECL_H
#define TACC_DECL_H

#include "dynarray.h"
#include "dynstring.h"
#include "expr.h"
#include "statement.h"
#include "string_list.h"

enum tacc_decl_kind { DECL_FUNCTION_DEF, DECL_DECLARATORS };
enum tacc_declarator_kind {
    DECLARATOR_PLAIN,
    DECLARATOR_ABSTRACT,
    DECLARATOR_SUB,
    DECLARATOR_ARRAY,
    DECLARATOR_FUNC
};
enum tacc_storage_class {
    STORAGE_UNSPECIFIED,
    STORAGE_REGISTER,
    STORAGE_AUTO,
    STORAGE_EXTERN,
    STORAGE_STATIC,
    STORAGE_TYPEDEF
};

union tacc_declarator_extra {
    /* owning */
    struct tacc_string *name;
    /* owning */
    struct tacc_declarator *sub_declarator;
    /* owning */
    struct tacc_array_declarator *arr_decl;
    /* owning */
    struct tacc_function_declarator *func_decl;
};

enum {
    TYPESPEC_UNSIGNED = 0x1,
    TYPESPEC_SIGNED = 0x2,
    TYPESPEC_BOOL = 0x4,
    TYPESPEC_CHAR = 0x8,
    TYPESPEC_SHORT = 0x10,
    TYPESPEC_INT = 0x20,
    TYPESPEC_LONG = 0x40,
    TYPESPEC_LONG_2 = 0x80,
    TYPESPEC_FLOAT = 0x100,
    TYPESPEC_DOUBLE = 0x200,
    TYPESPEC_COMPLEX = 0x400,
    TYPESPEC_IMAGINARY = 0x800,
    TYPESPEC_VOID = 0x1000,

    TYPESPEC_ENUM = 0x2000,
    TYPESPEC_STRUCT = 0x4000,
    TYPESPEC_UNION = 0x8000,
    TYPESPEC_TYPEDEF = 0x10000,

    TYPESPEC_INLINE = 0x20000,

    TYPEQUAL_VOLATILE = 0x40000,
    TYPEQUAL_RESTRICT = 0x80000,
    TYPEQUAL_CONST = 0x100000
};

struct tacc_enumerator {
    /* owning */
    struct tacc_string *name;

    /* owning */
    struct tacc_expr *value;
};

struct tacc_struct_declarator {
    /* owning, nullable (if this is a bitfield) */
    struct tacc_declarator *underlying;

    /* owning, nullable */
    struct tacc_expr *bitfield_size;
};

struct tacc_struct_decl {
    /* owning */
    struct tacc_decl_type *base_type;

    /* owning */
    struct tacc_struct_declarator_list *declarators;
};

struct tacc_decl_type {
    uint32_t spec_qual_flags;

    /* owning */
    struct tacc_string *referenced_name;

    /* owning */
    struct tacc_attribute_list *attributes;

    union {
        /* owning */
        struct tacc_struct_decl_list *struct_fields;
        /* owning */
        struct tacc_enumerator_list *enumerators;
    } extra;
};

struct tacc_function_param {
    /* owning */
    struct tacc_decl_type *base_type;

    /* owning */
    struct tacc_declarator *decl;
};

enum tacc_function_param_list_kind {
    FUNCPARAM_EMPTY_LIST,
    FUNCPARAM_VOID,
    FUNCPARAM_LIST,
    FUNCPARAM_LIST_VARARG,
    FUNCPARAM_OLD_STYLE_LIST
};

struct tacc_function_declarator {
    /* owning */
    struct tacc_declarator *sub_declarator;

    enum tacc_function_param_list_kind param_list_kind;
    union {
        struct tacc_function_param_list *modern_params;
        struct tacc_string_list *old_style_params;
    } param_list;
};

enum tacc_array_dim_kind {
    ARRAYDIM_UNSPECIFIED,     /* a[] */
    ARRAYDIM_UNSPECIFIED_VLA, /* a[*] */
    ARRAYDIM_EXPR             /* i.e. a[1] a[n + y] */
};

struct tacc_array_declarator {
    struct tacc_declarator *sub_declarator;
    enum tacc_array_dim_kind array_dim_kind;
    struct tacc_expr *dim_expr;
};

struct tacc_declarator {
    size_t indirection_level;
    enum tacc_declarator_kind kind;

    /* owning */
    struct tacc_attribute_list *attributes;

    union tacc_declarator_extra extra;
};

struct tacc_funcdef {
    /* owning */
    struct tacc_declarator *func_declaration;
    /* owning */
    struct tacc_decl_list *old_style_param_list;
    /* owning */
    struct tacc_compound_member_list *statements;
};

enum tacc_init_designator_kind {
    DESIGNATOR_NONE,
    DESIGNATOR_NAMED,
    DESIGNATOR_EXPR
};

struct tacc_sub_initializer {
    enum tacc_init_designator_kind designator_kind;
    union {
        /* owning */
        struct tacc_string *name;

        /* owning */
        struct tacc_expr *expr;
    } designator;

    /* owning */
    struct tacc_initializer *value;
};

struct tacc_initializer {
    tacc_bool plain_expr;

    union {
        /* owning */
        struct tacc_expr *expr;

        /* owning */
        struct tacc_sub_initializer_list *list;
    } value;
};

struct tacc_init_declarator {
    /* owning */
    struct tacc_declarator *declarator;

    /* owning */
    struct tacc_initializer *initializer;
};

struct tacc_decl {
    /* owning */
    struct tacc_decl_type *base_type;
    enum tacc_decl_kind kind;
    enum tacc_storage_class storage_class;

    /* owning */
    union {
        struct tacc_funcdef *func_def;
        struct tacc_init_declarator_list *declarators;
    } extra;
};

DECL_DYNARRAY_OVER(tacc_decl_list,
                   tacc_decl_list_entry,
                   struct tacc_decl *,
                   tacc_decl_list_new,
                   tacc_decl_list_init,
                   tacc_decl_list_get,
                   tacc_decl_list_push,
                   tacc_decl_list_pop,
                   tacc_decl_list_len,
                   tacc_decl_list_free)

DECL_DYNARRAY_OVER(tacc_declarator_list,
                   tacc_declarator_list_entry,
                   struct tacc_declarator *,
                   tacc_declarator_list_new,
                   tacc_declarator_list_init,
                   tacc_declarator_list_get,
                   tacc_declarator_list_push,
                   tacc_declarator_list_pop,
                   tacc_declarator_list_len,
                   tacc_declarator_list_free)

DECL_DYNARRAY_OVER(tacc_init_declarator_list,
                   tacc_init_declarator_list_entry,
                   struct tacc_init_declarator *,
                   tacc_init_declarator_list_new,
                   tacc_init_declarator_list_init,
                   tacc_init_declarator_list_get,
                   tacc_init_declarator_list_push,
                   tacc_init_declarator_list_pop,
                   tacc_init_declarator_list_len,
                   tacc_init_declarator_list_free)

DECL_DYNARRAY_OVER(tacc_sub_initializer_list,
                   tacc_sub_initializer_list_entry,
                   struct tacc_sub_initializer *,
                   tacc_sub_initializer_list_new,
                   tacc_sub_initializer_list_init,
                   tacc_sub_initializer_list_get,
                   tacc_sub_initializer_list_push,
                   tacc_sub_initializer_list_pop,
                   tacc_sub_initializer_list_len,
                   tacc_sub_initializer_list_free)

DECL_DYNARRAY_OVER(tacc_function_param_list,
                   tacc_function_param_list_entry,
                   struct tacc_function_param *,
                   tacc_function_param_list_new,
                   tacc_function_param_list_init,
                   tacc_function_param_list_get,
                   tacc_function_param_list_push,
                   tacc_function_param_list_pop,
                   tacc_function_param_list_len,
                   tacc_function_param_list_free)

DECL_DYNARRAY_OVER(tacc_enumerator_list,
                   tacc_enumerator_list_entry,
                   struct tacc_enumerator *,
                   tacc_enumerator_list_new,
                   tacc_enumerator_list_init,
                   tacc_enumerator_list_get,
                   tacc_enumerator_list_push,
                   tacc_enumerator_list_pop,
                   tacc_enumerator_list_len,
                   tacc_enumerator_list_free)

DECL_DYNARRAY_OVER(tacc_struct_declarator_list,
                   tacc_struct_declarator_list_entry,
                   struct tacc_struct_declarator *,
                   tacc_struct_declarator_list_new,
                   tacc_struct_declarator_list_init,
                   tacc_struct_declarator_list_get,
                   tacc_struct_declarator_list_push,
                   tacc_struct_declarator_list_pop,
                   tacc_struct_declarator_list_len,
                   tacc_struct_declarator_list_free)

DECL_DYNARRAY_OVER(tacc_struct_decl_list,
                   tacc_struct_decl_list_entry,
                   struct tacc_struct_decl *,
                   tacc_struct_decl_list_new,
                   tacc_struct_decl_list_init,
                   tacc_struct_decl_list_get,
                   tacc_struct_decl_list_push,
                   tacc_struct_decl_list_pop,
                   tacc_struct_decl_list_len,
                   tacc_struct_decl_list_free)

struct tacc_string *tacc_declarator_name(struct tacc_declarator *decl);
void tacc_function_param_free(struct tacc_function_param *param);
void tacc_funcdef_free(struct tacc_funcdef *func_def);
void tacc_enumerator_free(struct tacc_enumerator *enumerator);
void tacc_array_declarator_free(struct tacc_array_declarator *declarator);
void tacc_function_declarator_free(struct tacc_function_declarator *declarator);
void tacc_struct_declarator_free(
    struct tacc_struct_declarator *struct_declarator);
void tacc_initializer_free(struct tacc_initializer *initializer);
void tacc_sub_initializer_free(struct tacc_sub_initializer *sub_initializer);
void tacc_init_declarator_free(struct tacc_init_declarator *init_declarator);
void tacc_declarator_free(struct tacc_declarator *declarator);
void tacc_decl_type_free(struct tacc_decl_type *ty);
void tacc_struct_decl_free(struct tacc_struct_decl *decl);

struct tacc_enumerator *tacc_enumerator_new(void);
struct tacc_declarator *tacc_declarator_new(void);
struct tacc_init_declarator *tacc_init_declarator_new(void);
struct tacc_initializer *tacc_initializer_new(void);
struct tacc_sub_initializer *tacc_sub_initializer_new(void);
struct tacc_function_param *tacc_function_param_new(void);
struct tacc_array_declarator *tacc_array_declarator_new(void);
struct tacc_function_declarator *tacc_function_declarator_new(void);
struct tacc_struct_declarator *tacc_struct_declarator_new(void);
struct tacc_struct_decl *tacc_struct_decl_new(void);
struct tacc_funcdef *tacc_funcdef_new(void);
struct tacc_decl *tacc_decl_new(void);
void tacc_decl_free(struct tacc_decl *decl);

#endif

#ifndef TACC_TYPE_H
#define TACC_TYPE_H

#include "dynarray.h"
#include "dynstring.h"
#include "target_defs.h"
#include "util.h"

enum tacc_compound_type_kind {
    TYC_PTR,
    TYC_STRUCT,
    TYC_UNION,
    TYC_ENUM,
    TYC_ARRAY,
    TYC_ARRAY_FLEX,
    TYC_FN
};

enum tacc_type_kind {
    TYK_CHAR,
    TYK_UCHAR,
    TYK_SCHAR,
    TYK_USHORT,
    TYK_SSHORT,
    TYK_UINT,
    TYK_SINT,
    TYK_ULONG,
    TYK_SLONG,
    TYK_ULONGLONG,
    TYK_SLONGLONG,
    TYK_FLOAT,
    TYK_DOUBLE,
    TYK_LONGDOUBLE,

    TYK_BOOL,
    TYK_VOID,

    TYK_COMPOUND
};

enum tacc_int_rank {
    IRANK_BOOL,
    IRANK_CHAR,
    IRANK_SHORT,
    IRANK_INT,
    IRANK_LONG,
    IRANK_LLONG
};

DECL_DYNARRAY_OVER(tacc_struct_declaration_list,
                   tacc_struct_declaration_list_entry,
                   struct tacc_struct_declaration *,
                   tacc_struct_declaration_list_new,
                   tacc_struct_declaration_list_init,
                   tacc_struct_declaration_list_get,
                   tacc_struct_declaration_list_push,
                   tacc_struct_declaration_list_pop,
                   tacc_struct_declaration_list_len,
                   tacc_struct_declaration_list_free)

DECL_DYNARRAY_OVER(tacc_enum_declaration_list,
                   tacc_enum_declaration_list_entry,
                   struct tacc_enum_declaration *,
                   tacc_enum_declaration_list_new,
                   tacc_enum_declaration_list_init,
                   tacc_enum_declaration_list_get,
                   tacc_enum_declaration_list_push,
                   tacc_enum_declaration_list_pop,
                   tacc_enum_declaration_list_len,
                   tacc_enum_declaration_list_free)

enum tacc_function_param_list_kind {
    FUNCPARAM_EMPTY_LIST,
    FUNCPARAM_VOID,
    FUNCPARAM_LIST,
    FUNCPARAM_LIST_VARARG,
    FUNCPARAM_OLD_STYLE_LIST
};

struct tacc_function_type {
    enum tacc_function_param_list_kind param_list_kind;
    struct tacc_type_list *param_types;
    struct tacc_type *return_type;
};

struct tacc_array_type {
    /* borrow */
    struct tacc_type *element_type;
    /* owning */
    struct tacc_expr *dimension;
};

struct tacc_compound_type {
    enum tacc_compound_type_kind kind;

    /* owning */
    struct tacc_string *name;
    union {
        /* owning */
        struct tacc_struct_declaration_list *struct_union_decls;
        /* owning */
        struct tacc_enum_declaration_list *enum_decls;
        /* owning */
        struct tacc_function_type *function;
        /* owning */
        struct tacc_type *contained;
        /* owning */
        struct tacc_array_type *array;
    } extra;
};

struct tacc_type {
    enum tacc_type_kind kind;

    /* borrow */
    struct tacc_compound_type *extra;
};

DECL_DYNARRAY_OVER(tacc_compound_type_list,
                   tacc_compound_type_list_entry,
                   struct tacc_compound_type *,
                   tacc_compound_type_list_new,
                   tacc_compound_type_list_init,
                   tacc_compound_type_list_get,
                   tacc_compound_type_list_push,
                   tacc_compound_type_list_pop,
                   tacc_compound_type_list_len,
                   tacc_compound_type_list_free)

DECL_DYNARRAY_OVER(tacc_type_list,
                   tacc_type_list_entry,
                   struct tacc_type *,
                   tacc_type_list_new,
                   tacc_type_list_init,
                   tacc_type_list_get,
                   tacc_type_list_push,
                   tacc_type_list_pop,
                   tacc_type_list_len,
                   tacc_type_list_free)

struct tacc_type *tacc_type_new(void);
struct tacc_compound_type *tacc_compound_type_new(void);
struct tacc_array_type *tacc_array_type_new(void);
struct tacc_function_type *tacc_function_type_new(void);
struct tacc_type *tacc_get_basic_type(struct tacc_type_list *basic_types,
                                      enum tacc_type_kind kind);
tacc_bool tacc_type_kind_is_signed(enum tacc_type_kind kind,
                                   struct tacc_target *target);
tacc_bool tacc_type_is_subset(enum tacc_type_kind subset,
                              enum tacc_type_kind superset,
                              struct tacc_target *target);
size_t tacc_type_bit_width(struct tacc_target *target,
                           enum tacc_type_kind kind);
enum tacc_int_rank tacc_type_rank(enum tacc_type_kind kind);
enum tacc_type_kind tacc_type_to_unsigned(enum tacc_type_kind kind);
void tacc_struct_declaration_free(struct tacc_struct_declaration *decl);
void tacc_enum_declaration_free(struct tacc_enum_declaration *decl);
void tacc_type_free(struct tacc_type *type);
void tacc_compound_type_free(struct tacc_compound_type *type);

#endif

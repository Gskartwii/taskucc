#ifndef TACC_TYPE_H
#define TACC_TYPE_H

#include "dynarray.h"
#include "dynstring.h"

enum tacc_compound_type_kind {
    TYC_PTR,
    TYC_STRUCT,
    TYC_UNION,
    TYC_ENUM,
    TYC_ARRAY,
    TYC_ARRAY_FLEX,
    TYC_FN,

    TYC_TYPEDEF
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

struct tacc_compound_type {
    enum tacc_type_kind kind;
    struct tacc_type *contained;

    struct tacc_string *name;
    union {
        struct tacc_struct_declaration_list *struct_union_decls;
        struct tacc_enum_declaration_list *enum_decls;
    } declaration_list;
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

struct tacc_type_registry {
    struct tacc_compound_type_list *typedefs;
    struct tacc_compound_type_list *enums;
    struct tacc_compound_type_list *structs;
    struct tacc_compound_type_list *unions;
};

#endif

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
enum tacc_function_param_list_kind {
    FUNCPARAM_EMPTY_LIST,
    FUNCPARAM_VOID,
    FUNCPARAM_LIST,
    FUNCPARAM_LIST_VARARG,
    FUNCPARAM_OLD_STYLE_LIST
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

struct tacc_function_param {
    /* borrow */
    struct tacc_type *base_type;

    /* owning */
    struct tacc_declarator *decl;
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
    union tacc_declarator_extra extra;
};

struct tacc_funcdef {
    /* owning */
    struct tacc_declarator *func_declaration;
    /* owning */
    struct tacc_decl_list *old_style_param_list;
    /* owning */
    struct tacc_statement_list *statements;
};

struct tacc_decl {
    /* borrow */
    struct tacc_type *base_type;
    enum tacc_decl_kind kind;
    enum tacc_storage_class storage_class;

    /* owning */
    union {
        struct tacc_funcdef *func_def;
        struct tacc_declarator_list *declarators;
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

void tacc_function_param_free(struct tacc_function_param *param);
void tacc_funcdef_free(struct tacc_funcdef *func_def);
void tacc_array_declarator_free(struct tacc_array_declarator *declarator);
void tacc_function_declarator_free(struct tacc_function_declarator *declarator);
void tacc_declarator_free(struct tacc_declarator *declarator);
struct tacc_declarator *tacc_declarator_new(void);
struct tacc_function_param *tacc_function_param_new(void);
struct tacc_array_declarator *tacc_array_declarator_new(void);
struct tacc_function_declarator *tacc_function_declarator_new(void);
struct tacc_decl *tacc_decl_new(void);
void tacc_decl_free(struct tacc_decl *decl);

#endif

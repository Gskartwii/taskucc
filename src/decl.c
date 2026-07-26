#include "decl.h"
#include "dynarray.h"
#include "string_list.h"

MK_DYNARRAY_OVER(tacc_decl_list,
                 tacc_decl_list_entry,
                 struct tacc_decl *,
                 tacc_decl_list_new,
                 tacc_decl_list_init,
                 tacc_decl_list_get,
                 tacc_decl_list_push,
                 tacc_decl_list_pop,
                 tacc_decl_list_len,
                 tacc_decl_free,
                 tacc_decl_list_free)

MK_DYNARRAY_OVER(tacc_declarator_list,
                 tacc_declarator_list_entry,
                 struct tacc_declarator *,
                 tacc_declarator_list_new,
                 tacc_declarator_list_init,
                 tacc_declarator_list_get,
                 tacc_declarator_list_push,
                 tacc_declarator_list_pop,
                 tacc_declarator_list_len,
                 tacc_declarator_free,
                 tacc_declarator_list_free)

MK_DYNARRAY_OVER(tacc_function_param_list,
                 tacc_function_param_list_entry,
                 struct tacc_function_param *,
                 tacc_function_param_list_new,
                 tacc_function_param_list_init,
                 tacc_function_param_list_get,
                 tacc_function_param_list_push,
                 tacc_function_param_list_pop,
                 tacc_function_param_list_len,
                 tacc_function_param_free,
                 tacc_function_param_list_free)

void tacc_function_param_free(struct tacc_function_param *param) {
    tacc_declarator_free(param->decl);
    tacc_free(param);
}

void tacc_array_declarator_free(struct tacc_array_declarator *declarator) {
    if (declarator->dim_expr) {
        tacc_expr_free(declarator->dim_expr);
    }
    tacc_declarator_free(declarator->sub_declarator);
    tacc_free(declarator);
}

void tacc_function_declarator_free(
    struct tacc_function_declarator *declarator) {
    if (declarator->param_list_kind == FUNCPARAM_LIST ||
        declarator->param_list_kind == FUNCPARAM_LIST_VARARG) {
        tacc_function_param_list_free(declarator->param_list.modern_params);
        tacc_free(declarator->param_list.modern_params);
    } else if (declarator->param_list_kind == FUNCPARAM_OLD_STYLE_LIST) {
        tacc_string_list_free(declarator->param_list.old_style_params);
        tacc_free(declarator->param_list.old_style_params);
    }
    tacc_declarator_free(declarator->sub_declarator);
    tacc_free(declarator);
}

void tacc_declarator_free(struct tacc_declarator *declarator) {
    if (declarator->kind == DECLARATOR_ARRAY) {
        tacc_array_declarator_free(declarator->extra.arr_decl);
    } else if (declarator->kind == DECLARATOR_FUNC) {
        tacc_function_declarator_free(declarator->extra.func_decl);
    } else if (declarator->kind == DECLARATOR_PLAIN) {
        tacc_dynstring_free(declarator->extra.name);
    } else if (declarator->kind == DECLARATOR_SUB) {
        tacc_declarator_free(declarator->extra.sub_declarator);
    }

    tacc_free(declarator);
}

void tacc_funcdef_free(struct tacc_funcdef *func_def) {
    tacc_declarator_free(func_def->func_declaration);
    if (func_def->old_style_param_list) {
        tacc_decl_list_free(func_def->old_style_param_list);
        tacc_free(func_def->old_style_param_list);
    }
    tacc_statement_list_free(func_def->statements);
    tacc_free(func_def->statements);
    tacc_free(func_def);
}

struct tacc_function_param *tacc_function_param_new(void) {
    struct tacc_function_param *param;

    param = tacc_malloc(sizeof(struct tacc_function_param));
    param->base_type = NULL;
    param->decl = NULL;

    return param;
}

struct tacc_array_declarator *tacc_array_declarator_new(void) {
    struct tacc_array_declarator *declarator;

    declarator = tacc_malloc(sizeof(struct tacc_array_declarator));
    declarator->array_dim_kind = ARRAYDIM_UNSPECIFIED;
    declarator->dim_expr = NULL;
    declarator->sub_declarator = NULL;

    return declarator;
}

struct tacc_function_declarator *tacc_function_declarator_new(void) {
    struct tacc_function_declarator *declarator;

    declarator = tacc_malloc(sizeof(struct tacc_function_declarator));
    declarator->sub_declarator = NULL;
    declarator->param_list_kind = FUNCPARAM_EMPTY_LIST;

    return declarator;
}

struct tacc_declarator *tacc_declarator_new(void) {
    struct tacc_declarator *declarator;

    declarator = tacc_malloc(sizeof(struct tacc_declarator));
    declarator->kind = DECLARATOR_PLAIN;
    declarator->indirection_level = 0;

    return declarator;
}

struct tacc_decl *tacc_decl_new(void) {
    struct tacc_decl *decl;

    decl = tacc_malloc(sizeof(struct tacc_decl));
    decl->base_type = NULL;
    decl->kind = DECL_DECLARATORS;
    decl->storage_class = STORAGE_UNSPECIFIED;

    return decl;
}

void tacc_decl_free(struct tacc_decl *decl) {
    if (decl->kind == DECL_FUNCTION_DEF) {
        tacc_funcdef_free(decl->extra.func_def);
    } else if (decl->kind == DECL_DECLARATORS) {
        tacc_declarator_list_free(decl->extra.declarators);
        tacc_free(decl->extra.declarators);
    }
    tacc_free(decl);
}

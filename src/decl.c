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
    if (declarator->sub_declarator_kind == DECLARATOR_ARRAY) {
        tacc_array_declarator_free(declarator->extra.arr_decl);
    } else if (declarator->sub_declarator_kind == DECLARATOR_FUNC) {
        tacc_function_declarator_free(declarator->extra.func_decl);
    }
    tacc_free(declarator);
}

void tacc_function_declarator_free(
    struct tacc_function_declarator *declarator) {
    if (declarator->param_list_kind == FUNCPARAM_LIST ||
        declarator->param_list_kind == FUNCPARAM_LIST_VARARG) {
        tacc_function_param_list_free(declarator->param_list.modern_params);
    } else if (declarator->param_list_kind == FUNCPARAM_OLD_STYLE_LIST) {
        tacc_string_list_free(declarator->param_list.old_style_params);
    }
    tacc_free(declarator);
}

void tacc_declarator_free(struct tacc_declarator *declarator) {
    tacc_dynstring_free(declarator->name);

    if (declarator->kind == DECLARATOR_ARRAY) {
        tacc_array_declarator_free(declarator->extra.arr_decl);
    } else if (declarator->kind == DECLARATOR_FUNC) {
        tacc_function_declarator_free(declarator->extra.func_decl);
    }

    tacc_free(declarator);
}

void tacc_typedef_free(struct tacc_typedef *type_def) {
    tacc_declarator_list_free(type_def->defined_types);
    tacc_free(type_def);
}

void tacc_funcdef_free(struct tacc_funcdef *func_def) {
    tacc_declarator_free(func_def->func_declaration);
    if (func_def->old_style_param_list) {
        tacc_decl_list_free(func_def->old_style_param_list);
    }
    tacc_statement_list_free(func_def->statements);
    tacc_free(func_def);
}

void tacc_decl_free(struct tacc_decl *decl) {
    if (decl->kind == DECL_FUNCTION_DEF) {
        tacc_funcdef_free(decl->extra.func_def);
    } else if (decl->kind == DECL_TYPEDEF) {
        tacc_typedef_free(decl->extra.type_def);
    } else if (decl->kind == DECL_DECLARATORS) {
        tacc_declarator_list_free(decl->extra.declarators);
    }
    tacc_free(decl);
}

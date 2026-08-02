#include "decl.h"
#include "attribute.h"
#include "dynarray.h"
#include "statement.h"
#include "string_list.h"
#include "type.h"

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

MK_DYNARRAY_OVER(tacc_enumerator_list,
                 tacc_enumerator_list_entry,
                 struct tacc_enumerator *,
                 tacc_enumerator_list_new,
                 tacc_enumerator_list_init,
                 tacc_enumerator_list_get,
                 tacc_enumerator_list_push,
                 tacc_enumerator_list_pop,
                 tacc_enumerator_list_len,
                 tacc_enumerator_free,
                 tacc_enumerator_list_free)

MK_DYNARRAY_OVER(tacc_struct_decl_list,
                 tacc_struct_decl_list_entry,
                 struct tacc_struct_decl *,
                 tacc_struct_decl_list_new,
                 tacc_struct_decl_list_init,
                 tacc_struct_decl_list_get,
                 tacc_struct_decl_list_push,
                 tacc_struct_decl_list_pop,
                 tacc_struct_decl_list_len,
                 tacc_struct_decl_free,
                 tacc_struct_decl_list_free)

MK_DYNARRAY_OVER(tacc_struct_declarator_list,
                 tacc_struct_declarator_list_entry,
                 struct tacc_struct_declarator *,
                 tacc_struct_declarator_list_new,
                 tacc_struct_declarator_list_init,
                 tacc_struct_declarator_list_get,
                 tacc_struct_declarator_list_push,
                 tacc_struct_declarator_list_pop,
                 tacc_struct_declarator_list_len,
                 tacc_struct_declarator_free,
                 tacc_struct_declarator_list_free)

void tacc_function_param_free(struct tacc_function_param *param) {
    tacc_decl_type_free(param->base_type);
    tacc_declarator_free(param->decl);
    tacc_free(param);
}

void tacc_enumerator_free(struct tacc_enumerator *enumerator) {
    tacc_dynstring_free(enumerator->name);
    if (enumerator->value != NULL) {
        tacc_expr_free(enumerator->value);
    }
    tacc_free(enumerator);
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

    tacc_attribute_list_free(declarator->attributes);
    tacc_free(declarator->attributes);

    tacc_free(declarator);
}

void tacc_funcdef_free(struct tacc_funcdef *func_def) {
    tacc_declarator_free(func_def->func_declaration);
    if (func_def->old_style_param_list) {
        tacc_decl_list_free(func_def->old_style_param_list);
        tacc_free(func_def->old_style_param_list);
    }
    tacc_compound_member_list_free(func_def->statements);
    tacc_free(func_def->statements);
    tacc_free(func_def);
}

struct tacc_enumerator *tacc_enumerator_new(void) {
    struct tacc_enumerator *enumerator;

    enumerator = tacc_malloc(sizeof(struct tacc_enumerator));
    enumerator->name = NULL;
    enumerator->value = NULL;

    return enumerator;
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

struct tacc_struct_declarator *tacc_struct_declarator_new(void) {
    struct tacc_struct_declarator *struct_declarator;

    struct_declarator = tacc_malloc(sizeof(struct tacc_struct_declarator));
    struct_declarator->underlying = NULL;
    struct_declarator->bitfield_size = NULL;

    return struct_declarator;
}

struct tacc_struct_decl *tacc_struct_decl_new(void) {
    struct tacc_struct_decl *struct_decl;

    struct_decl = tacc_malloc(sizeof(struct tacc_struct_decl));
    struct_decl->declarators = tacc_struct_declarator_list_new();
    struct_decl->base_type = NULL;

    return struct_decl;
}

struct tacc_declarator *tacc_declarator_new(void) {
    struct tacc_declarator *declarator;

    declarator = tacc_malloc(sizeof(struct tacc_declarator));
    declarator->kind = DECLARATOR_PLAIN;
    declarator->indirection_level = 0;
    declarator->attributes = tacc_attribute_list_new();

    return declarator;
}

struct tacc_funcdef *tacc_funcdef_new(void) {
    struct tacc_funcdef *funcdef;

    funcdef = tacc_malloc(sizeof(struct tacc_funcdef));
    funcdef->func_declaration = NULL;
    funcdef->old_style_param_list = NULL;
    funcdef->statements = tacc_compound_member_list_new();

    return funcdef;
}

struct tacc_decl *tacc_decl_new(void) {
    struct tacc_decl *decl;

    decl = tacc_malloc(sizeof(struct tacc_decl));
    decl->base_type = NULL;
    decl->kind = DECL_DECLARATORS;
    decl->storage_class = STORAGE_UNSPECIFIED;

    return decl;
}

void tacc_struct_decl_free(struct tacc_struct_decl *decl) {
    tacc_struct_declarator_list_free(decl->declarators);
    tacc_free(decl->declarators);
    tacc_decl_type_free(decl->base_type);
    tacc_free(decl);
}

void tacc_struct_declarator_free(
    struct tacc_struct_declarator *struct_declarator) {
    if (struct_declarator->bitfield_size != NULL) {
        tacc_expr_free(struct_declarator->bitfield_size);
    }
    if (struct_declarator->underlying != NULL) {
        tacc_declarator_free(struct_declarator->underlying);
    }
    tacc_free(struct_declarator);
}

void tacc_decl_free(struct tacc_decl *decl) {
    tacc_decl_type_free(decl->base_type);
    if (decl->kind == DECL_FUNCTION_DEF) {
        tacc_funcdef_free(decl->extra.func_def);
    } else if (decl->kind == DECL_DECLARATORS) {
        tacc_declarator_list_free(decl->extra.declarators);
        tacc_free(decl->extra.declarators);
    }
    tacc_free(decl);
}

struct tacc_string *
tacc_declarator_name(struct tacc_declarator *declarator_in) {
    struct tacc_declarator *declarator;

    declarator = declarator_in;

    while (1) {
        switch (declarator->kind) {
        case DECLARATOR_PLAIN:
            return declarator->extra.name;
        case DECLARATOR_ABSTRACT:
            return NULL;
        case DECLARATOR_SUB:
            declarator = declarator->extra.sub_declarator;
            break;
        case DECLARATOR_ARRAY:
            declarator = declarator->extra.arr_decl->sub_declarator;
            break;
        case DECLARATOR_FUNC:
            declarator = declarator->extra.func_decl->sub_declarator;
            break;
        }
    }
}

void tacc_decl_type_free(struct tacc_decl_type *ty) {
    if ((ty->spec_qual_flags & (TYPESPEC_STRUCT | TYPESPEC_UNION)) != 0) {
        tacc_struct_decl_list_free(ty->extra.struct_fields);
        tacc_free(ty->extra.struct_fields);
    }
    if ((ty->spec_qual_flags & TYPESPEC_ENUM) != 0) {
        tacc_enumerator_list_free(ty->extra.enumerators);
        tacc_free(ty->extra.enumerators);
    }
    if (ty->referenced_name != NULL) {
        tacc_dynstring_free(ty->referenced_name);
    }
    tacc_attribute_list_free(ty->attributes);
    tacc_free(ty->attributes);
    tacc_free(ty);
}

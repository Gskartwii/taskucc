#include "format.h"
#include "decl.h"
#include "dynstring.h"
#include "expr.h"
#include <stdarg.h>

static void tacc_format_newline(struct tacc_formatter *fmt) {
    size_t i;
    printf("\n");

    for (i = 0; i < fmt->indent; i = i + 1) {
        printf(" ");
    }
}

static void tacc_format_indent(struct tacc_formatter *fmt) {
    fmt->indent = fmt->indent + 1;
}
static void tacc_format_deindent(struct tacc_formatter *fmt) {
    tacc_assert(fmt->indent > 0, "cannot deindent at base level");
    fmt->indent = fmt->indent - 1;
}

static void tacc_format_print(struct tacc_formatter *fmt, char *format, ...) {
    va_list list;

#ifndef __M2__
    (void) fmt;
#endif

    va_start(list, format);
    vprintf(format, list);
}

static void tacc_format_begin_scope(struct tacc_formatter *fmt, char *name) {
    tacc_format_print(fmt, "(");
    tacc_format_print(fmt, name);
    tacc_format_indent(fmt);
}
static void tacc_format_end_scope(struct tacc_formatter *fmt) {
    tacc_format_print(fmt, ")");
    tacc_format_deindent(fmt);
}
static void tacc_format_field_name(struct tacc_formatter *fmt, char *name) {
    tacc_format_newline(fmt);
    tacc_format_print(fmt, "#:%s ", name);
}

static void tacc_format_type(struct tacc_formatter *fmt, struct tacc_type *ty) {
    switch (ty->kind) {
    case TYK_CHAR:
        tacc_format_print(fmt, "char");
        break;
    case TYK_UCHAR:
        tacc_format_print(fmt, "unsigned-char");
        break;
    case TYK_SCHAR:
        tacc_format_print(fmt, "signed-char");
        break;
    case TYK_USHORT:
        tacc_format_print(fmt, "unsigned-short");
        break;
    case TYK_SSHORT:
        tacc_format_print(fmt, "signed-short");
        break;
    case TYK_UINT:
        tacc_format_print(fmt, "unsigned-int");
        break;
    case TYK_SINT:
        tacc_format_print(fmt, "signed-int");
        break;
    case TYK_ULONG:
        tacc_format_print(fmt, "unsigned-long");
        break;
    case TYK_SLONG:
        tacc_format_print(fmt, "signed-long");
        break;
    case TYK_ULONGLONG:
        tacc_format_print(fmt, "unsigned-long-long");
        break;
    case TYK_SLONGLONG:
        tacc_format_print(fmt, "signed-long-long");
        break;
    case TYK_FLOAT:
        tacc_format_print(fmt, "float");
        break;
    case TYK_DOUBLE:
        tacc_format_print(fmt, "double");
        break;
    case TYK_LONGDOUBLE:
        tacc_format_print(fmt, "long-double");
        break;
    case TYK_BOOL:
        tacc_format_print(fmt, "_Bool");
        break;
    case TYK_VOID:
        tacc_format_print(fmt, "void");
        break;
    case TYK_COMPOUND:
        tacc_assert(0, "TODO: compound type fmt");
        break;
    }
}

static void tacc_format_number(struct tacc_formatter *fmt,
                               struct tacc_u64 *number) {
    tacc_format_print(fmt, "0x%x:%x", number->high, number->low);
}

static void tacc_format_expr(struct tacc_formatter *fmt,
                             struct tacc_expr *expr) {
    size_t n;
    size_t i;
    struct tacc_expr_list_entry *entry;

    switch (expr->kind) {
    case EX_UNINIT:
        tacc_format_begin_scope(fmt, "uninit-expr");
        n = 0;
        break;
    case EX_NUM_LIT:
        tacc_format_begin_scope(fmt, "num-lit-expr");
        tacc_format_field_name(fmt, "val");
        tacc_format_number(fmt, expr->extra.const_val->value.int_value);
        tacc_format_field_name(fmt, "ty");
        tacc_format_type(fmt, expr->extra.const_val->type);
        n = 0;
        break;
    case EX_STRING_LIT:
        tacc_format_begin_scope(fmt, "string-lit-expr");
        /* skip formatting the string itself, for now */
        n = 0;
        break;
    case EX_IDENT:
        tacc_format_begin_scope(fmt, "ident-expr");
        tacc_format_field_name(fmt, "name");
        tacc_format_print(fmt, "%s", tacc_dynstring_as_str(expr->extra.name));
        n = 0;
        break;
    case EX_ADD:
        tacc_format_begin_scope(fmt, "+");
        n = 2;
        break;
    case EX_SUB:
        tacc_format_begin_scope(fmt, "-");
        n = 2;
        break;
    case EX_MUL:
        tacc_format_begin_scope(fmt, "*");
        n = 2;
        break;
    case EX_DIV:
        tacc_format_begin_scope(fmt, "/");
        n = 2;
        break;
    case EX_REM:
        tacc_format_begin_scope(fmt, "%");
        n = 2;
        break;
    case EX_POS:
        tacc_format_begin_scope(fmt, "un+");
        n = 1;
        break;
    case EX_NEG:
        tacc_format_begin_scope(fmt, "un-");
        n = 1;
        break;
    case EX_BAND:
        tacc_format_begin_scope(fmt, "&");
        n = 2;
        break;
    case EX_BOR:
        tacc_format_begin_scope(fmt, "|");
        n = 2;
        break;
    case EX_BXOR:
        tacc_format_begin_scope(fmt, "^");
        n = 2;
        break;
    case EX_BNOT:
        tacc_format_begin_scope(fmt, "un~");
        n = 1;
        break;
    case EX_SHL:
        tacc_format_begin_scope(fmt, "<<");
        n = 2;
        break;
    case EX_SHR:
        tacc_format_begin_scope(fmt, ">>");
        n = 2;
        break;
    case EX_AND:
        tacc_format_begin_scope(fmt, "&&");
        n = 2;
        break;
    case EX_OR:
        tacc_format_begin_scope(fmt, "||");
        n = 2;
        break;
    case EX_NOT:
        tacc_format_begin_scope(fmt, "!");
        n = 1;
        break;
    case EX_EQ:
        tacc_format_begin_scope(fmt, "==");
        n = 2;
        break;
    case EX_NE:
        tacc_format_begin_scope(fmt, "!=");
        n = 2;
        break;
    case EX_LE:
        tacc_format_begin_scope(fmt, "<=");
        n = 2;
        break;
    case EX_LT:
        tacc_format_begin_scope(fmt, "<");
        n = 2;
        break;
    case EX_GE:
        tacc_format_begin_scope(fmt, ">=");
        n = 2;
        break;
    case EX_GT:
        tacc_format_begin_scope(fmt, ">");
        n = 2;
        break;
    case EX_ASSI:
        tacc_format_begin_scope(fmt, "=");
        n = 2;
        break;
    case EX_ADD_ASSI:
        tacc_format_begin_scope(fmt, "+=");
        n = 2;
        break;
    case EX_SUB_ASSI:
        tacc_format_begin_scope(fmt, "-=");
        n = 2;
        break;
    case EX_MUL_ASSI:
        tacc_format_begin_scope(fmt, "*=");
        n = 2;
        break;
    case EX_DIV_ASSI:
        tacc_format_begin_scope(fmt, "/=");
        n = 2;
        break;
    case EX_REM_ASSI:
        tacc_format_begin_scope(fmt, "%=");
        n = 2;
        break;
    case EX_BAND_ASSI:
        tacc_format_begin_scope(fmt, "&=");
        n = 2;
        break;
    case EX_BOR_ASSI:
        tacc_format_begin_scope(fmt, "|=");
        n = 2;
        break;
    case EX_BXOR_ASSI:
        tacc_format_begin_scope(fmt, "^=");
        n = 2;
        break;
    case EX_LSH_ASSI:
        tacc_format_begin_scope(fmt, "<<=");
        n = 2;
        break;
    case EX_RSH_ASSI:
        tacc_format_begin_scope(fmt, ">>=");
        n = 2;
        break;
    case EX_INCR_PRE:
        tacc_format_begin_scope(fmt, "pre++");
        n = 1;
        break;
    case EX_DECR_PRE:
        tacc_format_begin_scope(fmt, "pre--");
        n = 1;
        break;
    case EX_INCR_POST:
        tacc_format_begin_scope(fmt, "post++");
        n = 1;
        break;
    case EX_DECR_POST:
        tacc_format_begin_scope(fmt, "post--");
        n = 1;
        break;
    case EX_SUBSCRIPT:
        tacc_format_begin_scope(fmt, "subscript");
        n = 2;
        break;
    case EX_DEREF:
        tacc_format_begin_scope(fmt, "*");
        n = 1;
        break;
    case EX_ADDROF:
        tacc_format_begin_scope(fmt, "&");
        n = 1;
        break;
    case EX_MEMBER:
        tacc_format_begin_scope(fmt, ".");
        tacc_format_field_name(fmt, "field");
        tacc_format_print(fmt, "%s", expr->extra.name);
        n = 1;
        break;
    case EX_PTR_MEMBER:
        tacc_format_begin_scope(fmt, "->");
        tacc_format_field_name(fmt, "field");
        tacc_format_print(fmt, "%s", expr->extra.name);
        n = 1;
        break;
    case EX_CALL:
        tacc_format_begin_scope(fmt, "call");
        tacc_format_field_name(fmt, "args");
        tacc_format_begin_scope(fmt, "args");
        for (i = 0; i < tacc_expr_list_len(expr->extra.op_list); i = i + 1) {
            entry = tacc_expr_list_get(expr->extra.op_list, i);
            tacc_format_expr(fmt, entry->content);
        }
        tacc_format_end_scope(fmt);
        n = 1;
        break;
    case EX_COMMA:
        tacc_format_begin_scope(fmt, "comma");
        tacc_format_field_name(fmt, "args");
        tacc_format_begin_scope(fmt, "args");
        for (i = 0; i < tacc_expr_list_len(expr->extra.op_list); i = i + 1) {
            entry = tacc_expr_list_get(expr->extra.op_list, i);
            tacc_format_expr(fmt, entry->content);
        }
        tacc_format_end_scope(fmt);
        n = 0;
        break;
    case EX_CAST:
        tacc_format_begin_scope(fmt, "cast");
        tacc_format_field_name(fmt, "ty");
        tacc_format_type(fmt, expr->extra.type);
        n = 1;
        break;
    case EX_SIZEOF:
        tacc_format_begin_scope(fmt, "sizeof");
        n = 1;
        break;
    case EX_SIZEOF_TY:
        tacc_format_begin_scope(fmt, "sizeof");
        tacc_format_field_name(fmt, "ty");
        tacc_format_type(fmt, expr->extra.type);
        n = 0;
        break;
    case EX_SELECT:
        tacc_format_begin_scope(fmt, "?:");
        n = 3;
        break;
    case EX_COMPOUND_LIT:
        tacc_assert(0, "TODO: format compound literal");
        return;
    default:
        tacc_assert(0, "invalid expression");
        return;
    }

    if (n > 0) {
        tacc_format_field_name(fmt, "op1");
        tacc_format_expr(fmt, expr->op1);
    }
    if (n > 1) {
        tacc_format_field_name(fmt, "op2");
        tacc_format_expr(fmt, expr->op2);
    }
    if (n > 2) {
        tacc_format_field_name(fmt, "op3");
        tacc_format_expr(fmt, expr->op3);
    }

    tacc_format_end_scope(fmt);
}

static void tacc_format_declarator(struct tacc_formatter *fmt,
                                   struct tacc_declarator *declarator) {
    size_t i;
    struct tacc_string_list_entry *entry;
    struct tacc_function_param_list_entry *funcparam_entry;

    tacc_format_begin_scope(fmt, "declarator");

    tacc_format_field_name(fmt, "indirection-level");
    tacc_format_print(fmt, "%u", declarator->indirection_level);
    switch (declarator->kind) {
    case DECLARATOR_SUB:
        tacc_format_field_name(fmt, "sub-declarator");
        tacc_format_declarator(fmt, declarator->extra.sub_declarator);
        break;
    case DECLARATOR_ABSTRACT:
        tacc_format_field_name(fmt, "abstract");
        break;
    case DECLARATOR_PLAIN:
        tacc_format_field_name(fmt, "plain-name");
        tacc_format_print(
            fmt, "%s", tacc_dynstring_as_str(declarator->extra.name));
        break;
    case DECLARATOR_ARRAY:
        tacc_format_field_name(fmt, "array-sub-declarator");
        tacc_format_declarator(fmt, declarator->extra.arr_decl->sub_declarator);
        tacc_format_field_name(fmt, "array-dim");
        if (declarator->extra.arr_decl->array_dim_kind ==
            ARRAYDIM_UNSPECIFIED) {
            tacc_format_print(fmt, "unspecified");
        } else if (declarator->extra.arr_decl->array_dim_kind ==
                   ARRAYDIM_UNSPECIFIED_VLA) {
            tacc_format_print(fmt, "unspecified-vla");
        } else {
            tacc_format_expr(fmt, declarator->extra.arr_decl->dim_expr);
        }
        break;
    case DECLARATOR_FUNC:
        tacc_format_field_name(fmt, "function-sub-declarator");
        tacc_format_declarator(fmt,
                               declarator->extra.func_decl->sub_declarator);
        tacc_format_field_name(fmt, "function-param-list");
        if (declarator->extra.func_decl->param_list_kind ==
            FUNCPARAM_EMPTY_LIST) {
            tacc_format_print(fmt, "empty");
        } else if (declarator->extra.func_decl->param_list_kind ==
                   FUNCPARAM_VOID) {
            tacc_format_print(fmt, "void");
        } else if (declarator->extra.func_decl->param_list_kind ==
                   FUNCPARAM_OLD_STYLE_LIST) {
            tacc_format_begin_scope(fmt, "old-style");
            for (i = 0;
                 i <
                 tacc_string_list_len(
                     declarator->extra.func_decl->param_list.old_style_params);
                 i = i + 1) {
                tacc_format_newline(fmt);
                entry = tacc_string_list_get(
                    declarator->extra.func_decl->param_list.old_style_params,
                    i);
                tacc_format_print(fmt, tacc_dynstring_as_str(entry->content));
            }
            tacc_format_end_scope(fmt);
        } else {
            if (declarator->extra.func_decl->param_list_kind ==
                FUNCPARAM_LIST_VARARG) {
                tacc_format_begin_scope(fmt, "param-list-va");
            } else {
                tacc_format_begin_scope(fmt, "param-list");
            }
            for (i = 0;
                 i < tacc_function_param_list_len(
                         declarator->extra.func_decl->param_list.modern_params);
                 i = i + 1) {
                funcparam_entry = tacc_function_param_list_get(
                    declarator->extra.func_decl->param_list.modern_params, i);
                tacc_format_newline(fmt);
                tacc_format_begin_scope(fmt, "param");
                tacc_format_field_name(fmt, "type");
                tacc_format_type(fmt, funcparam_entry->content->base_type);
                if (funcparam_entry->content->decl) {
                    tacc_format_field_name(fmt, "declarator");
                    tacc_format_declarator(fmt, funcparam_entry->content->decl);
                }
                tacc_format_end_scope(fmt);
            }
            tacc_format_end_scope(fmt);
        }
        break;
    }

    tacc_format_end_scope(fmt);
}

static void tacc_format_decl(struct tacc_formatter *fmt,
                             struct tacc_decl *decl) {
    size_t i;
    struct tacc_declarator_list_entry *entry;

    if (decl->kind == DECL_DECLARATORS) {
        tacc_format_begin_scope(fmt, "declaration");
    } else {
        tacc_format_begin_scope(fmt, "func-def");
    }

    tacc_format_field_name(fmt, "base-type");
    tacc_format_type(fmt, decl->base_type);
    tacc_format_field_name(fmt, "storage-class");
    switch (decl->storage_class) {
    case STORAGE_UNSPECIFIED:
        tacc_format_print(fmt, "unspecified");
        break;
    case STORAGE_REGISTER:
        tacc_format_print(fmt, "register");
        break;
    case STORAGE_AUTO:
        tacc_format_print(fmt, "auto");
        break;
    case STORAGE_EXTERN:
        tacc_format_print(fmt, "extern");
        break;
    case STORAGE_STATIC:
        tacc_format_print(fmt, "static");
        break;
    case STORAGE_TYPEDEF:
        tacc_format_print(fmt, "typedef");
        break;
    }

    if (decl->kind == DECL_DECLARATORS) {
        for (i = 0; i < tacc_declarator_list_len(decl->extra.declarators);
             i = i + 1) {
            tacc_format_newline(fmt);
            entry = tacc_declarator_list_get(decl->extra.declarators, i);
            tacc_format_declarator(fmt, entry->content);
        }
    }

    tacc_format_end_scope(fmt);
}

void tacc_format_ast(struct tacc_formatter *fmt, struct tacc_ast *ast) {
    size_t i;
    struct tacc_decl_list_entry *entry;

    for (i = 0; i < tacc_decl_list_len(ast->declarations); i = i + 1) {
        entry = tacc_decl_list_get(ast->declarations, i);
        tacc_format_decl(fmt, entry->content);
        tacc_format_newline(fmt);
    }
}

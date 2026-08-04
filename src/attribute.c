#include "attribute.h"
#include "expr.h"

MK_DYNARRAY_OVER(tacc_attribute_list,
                 tacc_attribute_list_entry,
                 struct tacc_attribute *,
                 tacc_attribute_list_new,
                 tacc_attribute_list_init,
                 tacc_attribute_list_get,
                 tacc_attribute_list_push,
                 tacc_attribute_list_pop,
                 tacc_attribute_list_len,
                 tacc_attribute_free,
                 tacc_attribute_list_free)

struct tacc_attribute *tacc_attribute_new(void) {
    struct tacc_attribute *attribute;

    attribute = tacc_malloc(sizeof(struct tacc_attribute));
    attribute->kind = ATTR_NORETURN;

    return attribute;
}

struct tacc_attribute_format *tacc_attribute_format_new(void) {
    struct tacc_attribute_format *attribute;

    attribute = tacc_malloc(sizeof(struct tacc_attribute_format));
    attribute->archetype = NULL;
    attribute->format_str_place = NULL;
    attribute->args_at = NULL;

    return attribute;
}

void tacc_attribute_format_free(struct tacc_attribute_format *attr) {
    tacc_dynstring_free(attr->archetype);
    tacc_expr_free(attr->format_str_place);
    tacc_expr_free(attr->args_at);
    tacc_free(attr);
}

void tacc_attribute_free(struct tacc_attribute *attr) {
    if (attr->kind == ATTR_FORMAT) {
        tacc_attribute_format_free(attr->extra.format);
    } else if (attr->kind == ATTR_ALIGNED) {
        tacc_expr_free(attr->extra.expr);
    }
    tacc_free(attr);
}

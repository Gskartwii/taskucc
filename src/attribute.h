#ifndef TACC_ATTRIBUTE_H
#define TACC_ATTRIBUTE_H

#include "dynarray.h"
#include "util.h"

enum tacc_attribute_kind { ATTR_ALIGNED, ATTR_FORMAT, ATTR_NORETURN };

struct tacc_attribute_format {
    /* owning */
    struct tacc_string *archetype;

    /* owning */
    struct tacc_expr *format_str_place;

    /* owning */
    struct tacc_expr *args_at;
};

struct tacc_attribute {
    enum tacc_attribute_kind kind;
    union {
        struct tacc_attribute_format *format;
        struct tacc_expr *expr;
    } extra;
};

DECL_DYNARRAY_OVER(tacc_attribute_list,
                   tacc_attribute_list_entry,
                   struct tacc_attribute *,
                   tacc_attribute_list_new,
                   tacc_attribute_list_init,
                   tacc_attribute_list_get,
                   tacc_attribute_list_push,
                   tacc_attribute_list_pop,
                   tacc_attribute_list_len,
                   tacc_attribute_list_free)

struct tacc_attribute *tacc_attribute_new(void);
struct tacc_attribute_format *tacc_attribute_format_new(void);

void tacc_attribute_format_free(struct tacc_attribute_format *attr);
void tacc_attribute_free(struct tacc_attribute *attr);

#endif

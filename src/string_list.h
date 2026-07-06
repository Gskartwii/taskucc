#ifndef TACC_STRING_LIST_H
#define TACC_STRING_LIST_H

#include "dynarray.h"
#include "dynstring.h"

DECL_DYNARRAY_OVER(tacc_string_list,
                   tacc_string_list_entry,
                   struct tacc_string *,
                   tacc_string_list_new,
                   tacc_string_list_init,
                   tacc_string_list_get,
                   tacc_string_list_push,
                   tacc_string_list_pop,
                   tacc_string_list_len,
                   tacc_string_list_free)

#endif

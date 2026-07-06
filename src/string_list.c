#include "string_list.h"
#include "util.h"

MK_DYNARRAY_OVER(tacc_string_list,
                 tacc_string_list_entry,
                 struct tacc_string *,
                 tacc_string_list_new,
                 tacc_string_list_init,
                 tacc_string_list_get,
                 tacc_string_list_push,
                 tacc_string_list_pop,
                 tacc_string_list_len,
                 tacc_dynstring_free,
                 tacc_string_list_free)

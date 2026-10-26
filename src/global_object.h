#ifndef TACC_GLOBAL_OBJECT_H
#define TACC_GLOBAL_OBJECT_H

#include "dynarray.h"
#include "dynhash.h"
#include "util.h"

struct tacc_global_object {
    uint32_t name_ref;

    tacc_bool is_enumerator;
    tacc_bool is_tentative;
    union {
        struct tacc_u64 *enumerator_value;
        struct tacc_type *obj_type;
    } extra;
};

DECL_DYNHASH_OVER_U32(tacc_global_object_map,
                      tacc_global_object_map_entry,
                      struct tacc_global_object *,
                      tacc_global_object_map_new,
                      tacc_global_object_map_init,
                      tacc_global_object_map_get,
                      tacc_global_object_map_insert,
                      tacc_global_object_map_fill_count,
                      tacc_global_object_map_free)

DECL_DYNARRAY_OVER(tacc_global_object_list,
                   tacc_global_object_list_entry,
                   struct tacc_global_object *,
                   tacc_global_object_list_new,
                   tacc_global_object_list_init,
                   tacc_global_object_list_get,
                   tacc_global_object_list_push,
                   tacc_global_object_list_pop,
                   tacc_global_object_list_len,
                   tacc_global_object_list_free)

struct tacc_global_object *tacc_global_object_new(void);
void tacc_global_object_free(struct tacc_global_object *obj);

#endif

#include "global_object.h"

MK_DYNHASH_OVER_U32(tacc_global_object_map,
                    name_ref,
                    tacc_global_object_map_entry,
                    struct tacc_global_object *,
                    tacc_global_object_map_new,
                    tacc_global_object_map_init,
                    tacc_global_object_map_get,
                    tacc_global_object_map_insert,
                    tacc_global_object_map_fill_count,
                    tacc_global_object_free,
                    tacc_global_object_map_free)

MK_DYNARRAY_OVER(tacc_global_object_list,
                 tacc_global_object_list_entry,
                 struct tacc_global_object *,
                 tacc_global_object_list_new,
                 tacc_global_object_list_init,
                 tacc_global_object_list_get,
                 tacc_global_object_list_push,
                 tacc_global_object_list_pop,
                 tacc_global_object_list_len,
                 tacc_global_object_free,
                 tacc_global_object_list_free)

struct tacc_global_object *tacc_global_object_new(void) {
    struct tacc_global_object *obj;

    obj = tacc_malloc(sizeof(struct tacc_global_object));
    obj->is_enumerator = 0;
    obj->is_tentative = 0;
    obj->name_ref = 0;

    return obj;
}

void tacc_global_object_free(struct tacc_global_object *obj) {
    if (obj->is_enumerator) {
        tacc_free(obj->extra.enumerator_value);
    }
    tacc_free(obj);
}

#include "call_itf.h"
#include "util.h"

struct tacc_callitf *tacc_callitf_new(void) {
    struct tacc_callitf *itf;

    itf = tacc_malloc(sizeof(struct tacc_callitf));
    itf->param_parts = tacc_callitf_part_list_new();
    itf->retval_kind = 0;
    itf->retval_reg = 0;
    itf->retval_reg_2 = 0;
    itf->retval_reg_class = 0;

    return itf;
}

struct tacc_callitf_part *tacc_callitf_part_new(void) {
    struct tacc_callitf_part *part;

    part = tacc_malloc(sizeof(struct tacc_callitf_part));
    part->place.kind = CALLITF_PLACE_REGISTER;
    part->place.extra.reg.reg = 0;
    part->place.extra.reg.reg_class = 0;
    part->offset_from_param_start = 0;
    part->param_idx = 0;

    return part;
}

void tacc_callitf_part_free(struct tacc_callitf_part *part) { tacc_free(part); }

MK_DYNARRAY_OVER(tacc_callitf_part_list,
                 tacc_callitf_part_list_entry,
                 struct tacc_callitf_part *,
                 tacc_callitf_part_list_new,
                 tacc_callitf_part_list_init,
                 tacc_callitf_part_list_get,
                 tacc_callitf_part_list_push,
                 tacc_callitf_part_list_pop,
                 tacc_callitf_part_list_len,
                 tacc_callitf_part_free,
                 tacc_callitf_part_list_free)

void tacc_callitf_free(struct tacc_callitf *itf) {
    tacc_callitf_part_list_free(itf->param_parts);
    tacc_free(itf->param_parts);
    tacc_free(itf);
}

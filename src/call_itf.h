#ifndef TACC_CALL_ITF_H
#define TACC_CALL_ITF_H

#include "dynarray.h"
#include <stddef.h>
#include <stdint.h>

enum tacc_callitf_place_kind {
    CALLITF_PLACE_REGISTER,
    CALLITF_PLACE_REGISTER_PAIR,
    CALLITF_PLACE_STACK,
};

enum tacc_callitf_retval_kind {
    CALLITF_RETVAL_NONE,
    CALLITF_RETVAL_REGISTER,
    CALLITF_RETVAL_REGISTER_PAIR,
    CALLITF_RETVAL_OUTPARAM,
};

struct tacc_callitf_place {
    enum tacc_callitf_place_kind kind;
    union {
        struct {
            uint32_t reg;
            uint32_t reg_class;
        } reg;
        struct {
            uint32_t reg;
            uint32_t reg_2;
            uint32_t reg_class;
        } pair;
        int stack_offset;
    } extra;
};

struct tacc_callitf_part {
    size_t param_idx;
    size_t offset_from_param_start;
    struct tacc_callitf_place place;
    struct tacc_type *ty;
};

DECL_DYNARRAY_OVER(tacc_callitf_part_list,
                   tacc_callitf_part_list_entry,
                   struct tacc_callitf_part *,
                   tacc_callitf_part_list_new,
                   tacc_callitf_part_list_init,
                   tacc_callitf_part_list_get,
                   tacc_callitf_part_list_push,
                   tacc_callitf_part_list_pop,
                   tacc_callitf_part_list_len,
                   tacc_callitf_part_list_free)

struct tacc_callitf {
    struct tacc_callitf_part_list *param_parts;
    enum tacc_callitf_retval_kind retval_kind;

    /*
     * when CALLITF_RETVAL_REGISTER_PAIR is used, two integer registers are used
     */
    uint32_t retval_reg;
    uint32_t retval_reg_2;
    uint32_t retval_reg_class;
};

struct tacc_callitf *tacc_callitf_new(void);
struct tacc_callitf_part *tacc_callitf_part_new(void);
void tacc_callitf_free(struct tacc_callitf *itf);

#endif

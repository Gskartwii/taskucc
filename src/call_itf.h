#ifndef TACC_CALL_ITF_H
#define TACC_CALL_ITF_H

#include "dynarray.h"
#include <stddef.h>
#include <stdint.h>

enum tacc_callitf_place_kind {
    CALLITF_PLACE_REGISTER,
    CALLITF_PLACE_FLOAT_REGISTER,
    CALLITF_PLACE_STACK,
    CALLITF_PLACE_FIRST_TARGET_SPECIAL,
};

enum tacc_callitf_retval_kind {
    CALLITF_RETVAL_NONE,
    CALLITF_RETVAL_REGISTER,
    CALLITF_RETVAL_FLOAT_REGISTER,
    CALLITF_RETVAL_REGISTER_PAIR,
    CALLITF_RETVAL_OUTPARAM,
    CALLITF_RETVAL_FIRST_TARGET_SPECIAL,
};

struct tacc_callitf_place {
    enum tacc_callitf_place_kind kind;
    union {
        struct {
            uint32_t reg;
        } reg;
        struct {
            int offset;
            size_t size;
            size_t align_p2;
        } stack;
    } extra;
};

struct tacc_callitf_part {
    size_t param_idx;
    size_t offset_from_param_start;
    struct tacc_callitf_place place;
    struct tacc_type *access_as_type;
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

    size_t implicit_stack_use;

    int frame_offset;
};

struct tacc_callitf *tacc_callitf_new(void);
struct tacc_callitf_part *tacc_callitf_part_new(void);
void tacc_callitf_free(struct tacc_callitf *itf);

#endif

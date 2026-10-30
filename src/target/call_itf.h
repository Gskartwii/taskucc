#ifndef TACC_TARGET_CALL_ITF_H
#define TACC_TARGET_CALL_ITF_H

#include "../call_itf.h"
#include "type.h"

void tacc_target_callitf_free(struct tacc_target_callitf *itf);
struct tacc_callitf *tacc_target_callitf_from_params(
    struct tacc_type_list *basic_types, struct tacc_function_type *ty);
struct tacc_callitf *
tacc_target_callitf_at_call_site(struct tacc_type_list *basic_types,
                                 struct tacc_function_type *ty,
                                 struct tacc_type_list *vararg_types);

#endif

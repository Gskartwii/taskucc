#ifndef TACC_TARGET_CALL_ITF_H
#define TACC_TARGET_CALL_ITF_H

#include "../call_itf.h"
#include "type.h"

struct tacc_callitf *
tacc_target_callitf_from_func_type(struct tacc_function_type *ty);

#endif

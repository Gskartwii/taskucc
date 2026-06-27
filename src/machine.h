#ifndef TACC_MACHINE_H
#define TACC_MACHINE_H

#include "soft_u64.h"
#include "type.h"

struct tacc_val {
    enum tacc_type_kind type_kind;
    struct tacc_compound_type *compound_type;

    union {
        struct tacc_u64 *int_value;
    } value;
};

struct tacc_val *tacc_val_new(void);
struct tacc_val *tacc_val_clone(struct tacc_val *val);
tacc_bool tacc_val_is_integral(struct tacc_val *val);

#endif

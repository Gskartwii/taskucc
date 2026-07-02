#ifndef TACC_MACHINE_H
#define TACC_MACHINE_H

#include "soft_u64.h"
#include "target_defs.h"
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
void tacc_val_free(struct tacc_val *val);
tacc_bool tacc_val_is_integral(struct tacc_val *val);
tacc_bool tacc_val_is_scalar(struct tacc_val *val);
tacc_bool tacc_val_is_arithmetic(struct tacc_val *val);
tacc_bool tacc_val_is_signed(struct tacc_val *val, struct tacc_target *target);
tacc_bool tacc_val_is_truthy(struct tacc_val *val);
tacc_bool tacc_val_is_eq(struct tacc_val *a, struct tacc_val *b);
void tacc_val_usual_arithmetic_conversions(struct tacc_val *a,
                                           struct tacc_val *b,
                                           struct tacc_target *target);
struct tacc_val *tacc_val_from_int(int value,
                                   enum tacc_type_kind kind,
                                   struct tacc_target *target);

#endif

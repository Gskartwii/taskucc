#ifndef TACC_MACHINE_H
#define TACC_MACHINE_H

#include "expr.h"
#include "soft_u64.h"
#include "target/target.h"
#include "type.h"

struct tacc_ptr_val {
    struct tacc_string *base_name;
    struct tacc_u64 *offset;
};

struct tacc_val {
    struct tacc_type *type;

    union {
        struct tacc_u64 *int_value;
        struct tacc_ptr_val *ptr_value;
    } value;
};

struct tacc_eval_context {
    struct tacc_target *target;
    struct tacc_type_list *basic_types;
    struct tacc_global_object_map *global_objects;
};

struct tacc_val *tacc_val_new(void);
struct tacc_ptr_val *tacc_ptr_val_new(void);
struct tacc_val *tacc_val_clone(struct tacc_val *val);
void tacc_val_free(struct tacc_val *val);
tacc_bool tacc_val_is_integral(struct tacc_val *val);
tacc_bool tacc_val_is_scalar(struct tacc_val *val);
tacc_bool tacc_val_is_arithmetic(struct tacc_val *val);
tacc_bool tacc_val_is_signed(struct tacc_val *val);
tacc_bool tacc_val_is_truthy(struct tacc_val *val);
tacc_bool tacc_val_is_negative(struct tacc_val *val);
tacc_bool tacc_val_is_eq(struct tacc_val *a, struct tacc_val *b);
void tacc_val_convert(struct tacc_val *val, struct tacc_type *into);
void tacc_val_usual_arithmetic_conversions(struct tacc_val *a,
                                           struct tacc_val *b,
                                           struct tacc_type_list *basic_types);
struct tacc_val *tacc_val_from_int(int value, struct tacc_type *ty);
struct tacc_val *tacc_val_zero(struct tacc_type *ty);
struct tacc_val *tacc_expr_const_eval(struct tacc_expr *expr,
                                      struct tacc_eval_context *ctx);

#endif

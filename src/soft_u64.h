#ifndef TACC_SOFT_LL_H
#define TACC_SOFT_LL_H

#include "util.h"
#include <stdint.h>

struct tacc_u64 {
    uint32_t low;
    uint32_t high;
};

struct tacc_u64 *tacc_u64_new(void);
struct tacc_u64 *tacc_u64_clone(struct tacc_u64 *orig);
struct tacc_u64 *tacc_u64_new_from_u32(uint32_t val);
void tacc_u64_zero(struct tacc_u64 *dst);
void tacc_u64_copy(struct tacc_u64 *dst, struct tacc_u64 *src);
/* returns resulting sign */
uint32_t tacc_u64_neg(struct tacc_u64 *dst, struct tacc_u64 *src);
/* returns carry */
uint32_t tacc_u64_add(struct tacc_u64 *to,
                      struct tacc_u64 *left,
                      struct tacc_u64 *right);
uint32_t tacc_u64_add_u32(struct tacc_u64 *to,
                          struct tacc_u64 *left,
                          uint32_t right);
/* returns carry */
uint32_t tacc_u64_sub(struct tacc_u64 *to,
                      struct tacc_u64 *left,
                      struct tacc_u64 *right);
uint32_t tacc_u64_sub_u32(struct tacc_u64 *to,
                          struct tacc_u64 *left,
                          uint32_t right);
void tacc_u64_mul(struct tacc_u64 *to,
                  struct tacc_u64 *left,
                  struct tacc_u64 *right);
void tacc_u64_mul_u32(struct tacc_u64 *to,
                      struct tacc_u64 *left,
                      uint32_t right);
void tacc_u64_lsh_n(struct tacc_u64 *to, struct tacc_u64 *left, int n);
void tacc_u64_lsh(struct tacc_u64 *to,
                  struct tacc_u64 *left,
                  struct tacc_u64 *right);
void tacc_u64_rsh_n(struct tacc_u64 *to, struct tacc_u64 *left, int n);
void tacc_u64_rsh(struct tacc_u64 *to,
                  struct tacc_u64 *left,
                  struct tacc_u64 *right);
void tacc_u64_arsh_n(struct tacc_u64 *to, struct tacc_u64 *left, int n);
void tacc_u64_arsh(struct tacc_u64 *to,
                   struct tacc_u64 *left,
                   struct tacc_u64 *right);
void tacc_u64_and(struct tacc_u64 *to,
                  struct tacc_u64 *left,
                  struct tacc_u64 *right);
void tacc_u64_or(struct tacc_u64 *to,
                 struct tacc_u64 *left,
                 struct tacc_u64 *right);
void tacc_u64_xor(struct tacc_u64 *to,
                  struct tacc_u64 *left,
                  struct tacc_u64 *right);
void tacc_u64_not(struct tacc_u64 *dst, struct tacc_u64 *src);
void tacc_u64_udiv(struct tacc_u64 *quot,
                   struct tacc_u64 *rem,
                   struct tacc_u64 *dividend,
                   struct tacc_u64 *divisor);
void tacc_u64_sdiv(struct tacc_u64 *quot,
                   struct tacc_u64 *rem,
                   struct tacc_u64 *dividend,
                   struct tacc_u64 *divisor);
void tacc_u64_from_u32(struct tacc_u64 *dst, uint32_t from);
void tacc_u64_from_i32(struct tacc_u64 *dst, int32_t from);
tacc_bool tacc_u64_is_zero(struct tacc_u64 *val);
tacc_bool tacc_u64_eq(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_ne(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_ult(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_ugt(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_ule(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_uge(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_slt(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_sgt(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_sle(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_sge(struct tacc_u64 *left, struct tacc_u64 *right);
tacc_bool tacc_u64_sign(struct tacc_u64 *src);

#endif

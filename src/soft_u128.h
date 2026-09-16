#ifndef TACC_SOFT_U128_H
#define TACC_SOFT_U128_H

#include "util.h"
#include <stdint.h>

struct tacc_u128 {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
};

void tacc_u128_from_limbs(
    struct tacc_u128 *n, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
tacc_bool tacc_u128_is_zero(struct tacc_u128 *n);
void tacc_u128_copy(struct tacc_u128 *dst, struct tacc_u128 *src);
int tacc_u128_clz(struct tacc_u128 *n);
void tacc_u128_lsh_n(struct tacc_u128 *dst, struct tacc_u128 *src, int n);
void tacc_u128_rsh_n(struct tacc_u128 *dst, struct tacc_u128 *src, int n);
void tacc_u128_or_u32(struct tacc_u128 *dst, struct tacc_u128 *a, uint32_t b);
void tacc_u128_add(struct tacc_u128 *dst,
                   struct tacc_u128 *a,
                   struct tacc_u128 *b);
void tacc_u128_add_u32(struct tacc_u128 *dst, struct tacc_u128 *a, uint32_t b);
void tacc_u128_sub(struct tacc_u128 *dst,
                   struct tacc_u128 *a,
                   struct tacc_u128 *b);

#endif

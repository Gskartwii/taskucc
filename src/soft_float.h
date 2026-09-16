#ifndef TACC_SOFT_FLOAT_H
#define TACC_SOFT_FLOAT_H

#include "soft_u128.h"
#include "util.h"
#include <stdint.h>

struct tacc_f128 {
    uint32_t mant_a;
    uint32_t mant_b;
    uint32_t mant_c;
    uint32_t mant_d;
    /* exponent: stored biased */
    uint16_t exponent;
    tacc_bool sign;
};

void tacc_f128_zero(struct tacc_f128 *f);
void tacc_f128_copy(struct tacc_f128 *dst, struct tacc_f128 *src);
void tacc_f128_epsilonl128(struct tacc_f128 *f);
tacc_bool tacc_f128_is_zero(struct tacc_f128 *f);
tacc_bool tacc_f128_is_nan(struct tacc_f128 *f);
tacc_bool tacc_f128_ge(struct tacc_f128 *left, struct tacc_f128 *right);
tacc_bool tacc_f128_ge_s32(struct tacc_f128 *left, int32_t x);
void tacc_f128_from_u32(struct tacc_f128 *f, uint32_t n);
void tacc_f128_from_s32(struct tacc_f128 *f, int32_t n);
void tacc_f128_from_frac(struct tacc_f128 *f, int dividend, int divisor);
void tacc_f128_scalbn(struct tacc_f128 *dst, struct tacc_f128 *src, int exp);
void tacc_f128_scalbnl(struct tacc_f128 *dst, struct tacc_f128 *src, int exp);
void tacc_f128_addl(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b);
void tacc_f128_subl(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b);
void tacc_f128_mull(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b);
void tacc_f128_divl(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b);
void tacc_f128_addl_u32(struct tacc_f128 *dst, struct tacc_f128 *a, uint32_t b);
void tacc_f128_subl_u32(struct tacc_f128 *dst, struct tacc_f128 *a, uint32_t b);
void tacc_f128_mull_u32(struct tacc_f128 *dst, struct tacc_f128 *a, uint32_t b);
void tacc_f128_divl_u32(struct tacc_f128 *dst, struct tacc_f128 *a, uint32_t b);
void tacc_f128_absl(struct tacc_f128 *dst, struct tacc_f128 *src);
void tacc_f128_copysignl(struct tacc_f128 *dst,
                         struct tacc_f128 *orig,
                         struct tacc_f128 *sign_src);
void tacc_f128_fmodl(struct tacc_f128 *dst,
                     struct tacc_f128 *dividend,
                     struct tacc_f128 *divisor);
void tacc_f128_round_f64(struct tacc_f128 *dst, struct tacc_f128 *src);

#endif

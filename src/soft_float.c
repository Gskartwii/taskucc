#include "soft_float.h"
#include "gcc_compat.h"
#include "util.h"

#define EXP_BIAS 16383
#define SIGNIFICANT_DIGITS 112
#define MAX_NORM_EXP 16383
#define MIN_NORM_EXP (-16382)
#define INF_EXPONENT 32767
#define DBL_INF_EXPONENT 1024
#define DBL_BIAS 1022
#define DBL_SIGNIFICANT_DIGITS 52

void tacc_f128_zero(struct tacc_f128 *f) {
    f->exponent = 0;
    f->mant_a = 0;
    f->mant_b = 0;
    f->mant_c = 0;
    f->mant_d = 0;
    f->sign = 0;
}
void tacc_f128_copy(struct tacc_f128 *dst, struct tacc_f128 *src) {
    dst->exponent = src->exponent;
    dst->mant_a = src->mant_a;
    dst->mant_b = src->mant_b;
    dst->mant_c = src->mant_c;
    dst->mant_d = src->mant_d;
    dst->sign = src->sign;
}

void tacc_f128_epsilonl128(struct tacc_f128 *f) {
    tacc_f128_zero(f);
    f->exponent = EXP_BIAS - SIGNIFICANT_DIGITS;
}

tacc_bool tacc_f128_is_zero(struct tacc_f128 *f) {
    return f->exponent == 0 && f->mant_a == 0 && f->mant_b == 0 &&
           f->mant_c == 0 && f->mant_d == 0;
}

tacc_bool tacc_f128_is_nan(struct tacc_f128 *f) {
    return f->exponent == INF_EXPONENT && (f->mant_a != 0 || f->mant_b != 0 ||
                                           f->mant_c != 0 || f->mant_d != 0);
}

tacc_bool tacc_f128_ge(struct tacc_f128 *left, struct tacc_f128 *right) {
    tacc_bool both_positive;

    if (tacc_f128_is_nan(left) || tacc_f128_is_nan(right)) {
        return 0;
    }
    if (tacc_f128_is_zero(left) && tacc_f128_is_zero(right)) {
        /* signed and unsigned zero are equal */
        return 1;
    }
    if (!left->sign && right->sign) {
        return 1;
    }
    if (left->sign && !right->sign) {
        return 0;
    }
    /* same sign */
    both_positive = !left->sign;

    if (left->exponent > right->exponent) {
        /*
         * abs val of left is bigger than that of right
         */
        return both_positive;
    }
    if (left->exponent < right->exponent) {
        return !both_positive;
    }
    /* exponents are equal */

    if (left->mant_a > right->mant_a) {
        return both_positive;
    }
    if (left->mant_a < right->mant_a) {
        return !both_positive;
    }
    if (left->mant_b > right->mant_b) {
        return both_positive;
    }
    if (left->mant_b < right->mant_b) {
        return !both_positive;
    }
    if (left->mant_c > right->mant_c) {
        return both_positive;
    }
    if (left->mant_c < right->mant_c) {
        return !both_positive;
    }
    if (left->mant_d > right->mant_d) {
        return both_positive;
    }
    if (left->mant_d < right->mant_d) {
        return !both_positive;
    }
    /* equal */
    return 1;
}

tacc_bool tacc_f128_ge_s32(struct tacc_f128 *left, int32_t x) {
    struct tacc_f128 aux;

    tacc_f128_from_s32(&aux, x);

    return tacc_f128_ge(left, &aux);
}

void tacc_f128_from_u32(struct tacc_f128 *f, uint32_t n) {
    int leading_zeroes;
    size_t i;

    tacc_f128_zero(f);

    if (n == 0) {
        return;
    }

    f->sign = 0;

    leading_zeroes = 0;
    for (i = 32; i > 0; i = i - 1) {
        if (((n >> (i - 1)) & 1) != 0) {
            break;
        }
        leading_zeroes = leading_zeroes + 1;
    }

    if (leading_zeroes < 31) {
        f->mant_a = n << (leading_zeroes + 1);
    }
    f->exponent = (uint16_t) (EXP_BIAS + 31 - leading_zeroes);
}

void tacc_f128_from_s32(struct tacc_f128 *f, int32_t n) {
    if (n < 0) {
        tacc_f128_from_u32(f, (uint32_t) (-n));
        f->sign = 1;
        return;
    }
    tacc_f128_from_u32(f, (uint32_t) n);
}

void tacc_f128_from_frac(struct tacc_f128 *f, int dividend, int divisor) {
    struct tacc_f128 aux;

    tacc_f128_from_s32(f, dividend);
    tacc_f128_from_s32(&aux, divisor);
    tacc_f128_divl(f, f, &aux);
}

void tacc_f128_scalbn(struct tacc_f128 *dst, struct tacc_f128 *src, int exp) {
    tacc_f128_scalbnl(dst, src, exp);
    tacc_f128_round_f64(dst, dst);
}

static void tacc_f128_normalize(struct tacc_f128 *f) {
    if (f->exponent > 32767) {
        /* cap exponent at infinities */
        f->exponent = 32767;
    }
    /* drop excess precision */
    f->mant_d &= ((unsigned) 0xFFFFFFFF) << (128 - 113);
}

static void tacc_f128_ldexp1(struct tacc_f128 *f, int exp) {
    int exp_mod;

    tacc_f128_zero(f);
    if (exp > EXP_BIAS) {
        f->exponent = INF_EXPONENT;
        return;
    } else if (exp < -16382) {
        /* subnormals */
        f->exponent = 0;
        exp_mod = exp + 16382;
        if (exp_mod > -32) {
            f->mant_a = ((unsigned) 1) << (unsigned) (32 + exp_mod);
        } else if (exp_mod > -64) {
            f->mant_b = ((unsigned) 1) << (unsigned) (64 + exp_mod);
        } else if (exp_mod > -96) {
            f->mant_c = ((unsigned) 1) << (unsigned) (96 + exp_mod);
        } else if (exp_mod > -112) {
            f->mant_d = ((unsigned) 1) << (unsigned) (128 + exp_mod);
        } else {
            /* out of exactly representable subnormal range, round */
        }
        return;
    }
    f->exponent = (uint16_t) (EXP_BIAS + exp);
    tacc_f128_normalize(f);
}

void tacc_f128_scalbnl(struct tacc_f128 *dst, struct tacc_f128 *src, int exp) {
    /* based on musl */
    struct tacc_f128 intermediate;
    struct tacc_f128 aux;
    struct tacc_f128 aux_2;
    int exp_mod;

    exp_mod = exp;
    if (exp_mod > MAX_NORM_EXP) {
        tacc_f128_ldexp1(&aux, MAX_NORM_EXP);
        tacc_f128_mull(&intermediate, src, &aux);
        exp_mod = exp_mod - MAX_NORM_EXP;
        if (exp_mod > MAX_NORM_EXP) {
            tacc_f128_mull(&intermediate, &intermediate, &aux);
            exp_mod = exp_mod - MAX_NORM_EXP;
            if (exp_mod > MAX_NORM_EXP) {
                exp_mod = MAX_NORM_EXP;
            }
        }
    } else if (exp_mod < MIN_NORM_EXP) {
        tacc_f128_ldexp1(&aux, MIN_NORM_EXP);
        tacc_f128_ldexp1(&aux_2, 113);
        tacc_f128_mull(&aux, &aux, &aux_2);
        tacc_f128_mull(&intermediate, src, &aux);
        exp_mod = exp_mod + (-MIN_NORM_EXP - 113);
        if (exp_mod < MIN_NORM_EXP) {
            tacc_f128_mull(&intermediate, &intermediate, &aux);
            exp_mod = exp_mod + (-MIN_NORM_EXP - 113);
            if (exp_mod < MIN_NORM_EXP) {
                exp_mod = MIN_NORM_EXP;
            }
        }
    } else {
        tacc_f128_copy(&intermediate, src);
    }

    tacc_f128_from_u32(&aux, 1);
    aux.exponent = (uint16_t) (EXP_BIAS + exp_mod);
    tacc_f128_mull(dst, &intermediate, &aux);
}

static int tacc_f128_unpack(struct tacc_f128 *f,
                            struct tacc_u128 *significand) {
    int exponent_adjusted;

    tacc_u128_from_limbs(
        significand, f->mant_a, f->mant_b, f->mant_c, f->mant_d);
    if (f->exponent == 0) {
        exponent_adjusted = -tacc_u128_clz(significand);
        tacc_u128_lsh_n(significand, significand, -exponent_adjusted);
    } else {
        exponent_adjusted = ((int) (f->exponent)) - EXP_BIAS;

        /* set implicit bit */
        tacc_u128_rsh_n(significand, significand, 1);
        significand->a = significand->a | 0x80000000;
    }

    return exponent_adjusted;
}

static void tacc_f128_pack(struct tacc_f128 *f,
                           struct tacc_u128 *significand,
                           int exponent,
                           tacc_bool sign) {
    int final_exponent;
    struct tacc_u128 lost;
    tacc_bool is_subnormal;

    final_exponent = exponent;

    is_subnormal = 0;
    if (final_exponent + EXP_BIAS < 0) {
        is_subnormal = 1;
        if (final_exponent + 128 + EXP_BIAS <= 0) {
            f->exponent = 0;
            f->mant_a = 0;
            f->mant_b = 0;
            f->mant_c = 0;
            f->mant_d = 0;
            f->sign = sign;
            return;
        }
        tacc_u128_lsh_n(&lost, significand, 32 + final_exponent + EXP_BIAS);
        tacc_u128_rsh_n(significand, significand, -(final_exponent + EXP_BIAS));
        if (!tacc_u128_is_zero(&lost)) {
            tacc_u128_or_u32(significand, significand, 1);
        }
    }

    if ((significand->d & 0xFFFF) > 0x8000 ||
        ((significand->d & 0xFFFF) == 0x8000 &&
         (significand->d & 0x10000) != 0)) {
        /* round up */

        /* make space to catch overflow */
        tacc_u128_rsh_n(significand, significand, 1);
        tacc_u128_add_u32(significand, significand, 0x10000);
        if ((significand->a >> 31) != 0) {
            if (is_subnormal) {
                /* overflow from subnormal to minimum normal number */
                f->exponent = 1;
                f->mant_a = 0;
                f->mant_b = 0;
                f->mant_c = 0;
                f->mant_d = 0;
                f->sign = sign;
                return;
            } else {
                final_exponent = final_exponent + 1;
            }
        } else {
            /* undo overflow guard */
            tacc_u128_lsh_n(significand, significand, 1);
        }
    }

    if (final_exponent > MAX_NORM_EXP) {
        f->exponent = INF_EXPONENT;
        f->mant_a = 0;
        f->mant_b = 0;
        f->mant_c = 0;
        f->mant_d = 0;
        f->sign = sign;
        return;
    }

    if (is_subnormal) {
        f->exponent = 0;
        f->mant_a = significand->a;
        f->mant_b = significand->b;
        f->mant_c = significand->c;
        f->mant_d = significand->d & 0xFFFF0000;
        f->sign = sign;
        return;
    }

    /* drop implicit bit */
    tacc_u128_lsh_n(significand, significand, 1);

    f->mant_a = significand->a;
    f->mant_b = significand->b;
    f->mant_c = significand->c;
    f->mant_d = significand->d & 0xFFFF0000;
    f->sign = sign;
    f->exponent = (uint16_t) (final_exponent + EXP_BIAS);
    f->sign = sign;
}

void tacc_f128_addl(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b) {
    tacc_bool sgn;
    struct tacc_f128 *far_f;
    struct tacc_f128 *near_f;
    struct tacc_f128 aux_f;
    struct tacc_f128 aux_f_2;
    struct tacc_u128 far_f_significand;
    struct tacc_u128 near_f_significand;
    struct tacc_u128 u128_aux;
    int far_exponent_adjusted;
    int near_exponent_adjusted;
    int exponent_delta;
    int exponent_adjust;
    int final_exponent;

    if (tacc_f128_is_nan(a)) {
        tacc_f128_copy(dst, a);
        return;
    }
    if (tacc_f128_is_nan(b)) {
        tacc_f128_copy(dst, b);
        return;
    }
    /* not NaN */

    if (a->exponent == INF_EXPONENT) {
        tacc_f128_copy(dst, a);
        if (b->exponent == INF_EXPONENT) {
            if (a->sign != b->sign) {
                dst->mant_a = ((unsigned) 1) << 31;
            }
        }
        return;
    }
    if (b->exponent == INF_EXPONENT) {
        tacc_f128_copy(dst, b);
        return;
    }
    /* not NaN or +-inf */

    if (tacc_f128_is_zero(a)) {
        if (tacc_f128_is_zero(b)) {
            sgn = a->sign && b->sign;
            tacc_f128_zero(dst);
            dst->sign = sgn;
            return;
        }
        tacc_f128_copy(dst, b);
        return;
    }
    if (tacc_f128_is_zero(b)) {
        tacc_f128_copy(dst, a);
        return;
    }
    /* not NaN or +-inf or +-0 */

    tacc_f128_absl(&aux_f, a);
    tacc_f128_absl(&aux_f_2, b);
    if (tacc_f128_ge(&aux_f, &aux_f_2)) {
        far_f = a;
        near_f = b;
    } else {
        far_f = b;
        near_f = a;
    }

    far_exponent_adjusted = tacc_f128_unpack(far_f, &far_f_significand);
    near_exponent_adjusted = tacc_f128_unpack(near_f, &near_f_significand);
    /*
     * Invariants:
     * - near_f_significand.a & (1<<31) is set, and the same holds for far.
     * - At most 113 MSBs of significands are set.
     */

    exponent_delta = far_exponent_adjusted - near_exponent_adjusted;
    if (exponent_delta >= 128) {
        /* underflow, set sticky and reset rest of bits */
        tacc_u128_from_limbs(&near_f_significand, 0, 0, 0, 0x10000);
    } else {
        /* save the bits that will be lost... */
        tacc_u128_lsh_n(&u128_aux, &near_f_significand, 128 - exponent_delta);
        tacc_u128_rsh_n(
            &near_f_significand, &near_f_significand, exponent_delta);

        /* lost bits? */
        if (!tacc_u128_is_zero(&u128_aux)) {
            /* ensure sticky bit is set to indicate underflow */
            tacc_u128_or_u32(&near_f_significand, &near_f_significand, 0x10000);
        }
    }

    if (near_f->sign != far_f->sign) {
        /* we are performing subtraction */
        tacc_u128_sub(&u128_aux, &far_f_significand, &near_f_significand);
        if (tacc_u128_is_zero(&u128_aux)) {
            /* Exact result. Skip setting sign and rounding. */
            tacc_f128_zero(dst);
            return;
        }
    } else {
        /* make space for overflow. this rsh never loses precision */
        tacc_u128_rsh_n(&far_f_significand, &far_f_significand, 1);
        tacc_u128_rsh_n(&near_f_significand, &near_f_significand, 1);

        tacc_u128_add(&u128_aux, &far_f_significand, &near_f_significand);

        if ((u128_aux.a >> 31) == 0) {
            /* didn't overflow, normalize */
            tacc_u128_lsh_n(&u128_aux, &u128_aux, 1);
        } else {
            /* overflowed, overall exponent becomes higher */
            far_exponent_adjusted = far_exponent_adjusted + 1;
        }
    }

    exponent_adjust = tacc_u128_clz(&u128_aux);
    final_exponent = exponent_adjust + far_exponent_adjusted;

    tacc_f128_pack(dst, &u128_aux, final_exponent, far_f->sign);
}

void tacc_f128_subl(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b) {
    struct tacc_f128 aux;

    tacc_f128_copy(&aux, b);
    aux.sign = !b->sign;
    tacc_f128_addl(dst, a, &aux);
}

void tacc_f128_mull(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b) {
    struct tacc_u128 a_significand;
    struct tacc_u128 b_significand;
    struct tacc_u128 product_high;
    struct tacc_u128 product_low;
    int a_exponent_adjusted;
    int b_exponent_adjusted;
    tacc_bool res_sign;

    if (tacc_f128_is_nan(a)) {
        tacc_f128_copy(dst, a);
        return;
    }
    if (tacc_f128_is_nan(b)) {
        tacc_f128_copy(dst, b);
        return;
    }
    /* not NaN */

    res_sign = 0;
    if (a->sign != b->sign) {
        res_sign = 1;
    }

    if (a->exponent == INF_EXPONENT) {
        tacc_f128_copy(dst, a);
        dst->sign = 0;
        if (tacc_f128_is_zero(b)) {
            dst->mant_a = ((unsigned) 1) << 31;
            return;
        }
        dst->sign = res_sign;
        return;
    }
    if (b->exponent == INF_EXPONENT) {
        tacc_f128_copy(dst, b);
        dst->sign = 0;
        if (tacc_f128_is_zero(a)) {
            dst->mant_a = ((unsigned) 1) << 31;
        }
        dst->sign = res_sign;
        return;
    }
    /* not NaN or +-inf */

    if (tacc_f128_is_zero(a)) {
        tacc_f128_copy(dst, a);
        dst->sign = res_sign;
        return;
    }
    if (tacc_f128_is_zero(b)) {
        tacc_f128_copy(dst, b);
        dst->sign = res_sign;
        return;
    }
    /* not NaN or +-inf or +-0 */

    a_exponent_adjusted = tacc_f128_unpack(a, &a_significand);
    b_exponent_adjusted = tacc_f128_unpack(b, &b_significand);

    tacc_u128_mul_widening(
        &product_high, &product_low, &a_significand, &b_significand);
    if (!tacc_u128_is_zero(&product_low)) {
        tacc_u128_or_u32(&product_high, &product_high, 1);
    }
    tacc_f128_pack(dst,
                   &product_high,
                   a_exponent_adjusted + b_exponent_adjusted,
                   res_sign);
}

void tacc_f128_divl(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b) {
    tacc_bool res_sign;
    struct tacc_u128 a_significand;
    struct tacc_u128 b_significand;
    struct tacc_u128 quotient_significand;
    int exponent;
    size_t i;
    tacc_bool sign;

    if (tacc_f128_is_nan(a)) {
        tacc_f128_copy(dst, a);
        return;
    }
    if (tacc_f128_is_nan(b)) {
        tacc_f128_copy(dst, b);
        return;
    }
    /* not NaN */

    res_sign = 0;
    if (a->sign != b->sign) {
        res_sign = 1;
    }

    if (a->exponent == INF_EXPONENT) {
        tacc_f128_copy(dst, a);
        dst->sign = 0;
        if (tacc_f128_is_zero(b)) {
            dst->mant_a = ((unsigned) 1) << 31;
            return;
        }
        dst->sign = res_sign;
        return;
    }
    if (b->exponent == INF_EXPONENT) {
        tacc_f128_zero(dst);
        dst->sign = res_sign;
        return;
    }
    /* not NaN or +-inf */

    if (tacc_f128_is_zero(b)) {
        dst->sign = res_sign;
        dst->exponent = INF_EXPONENT;
        dst->mant_a = 0;
        dst->mant_b = 0;
        dst->mant_c = 0;
        dst->mant_d = 0;
        if (tacc_f128_is_zero(a)) {
            dst->mant_a = ((unsigned) 1) << 31;
        }
        return;
    }
    if (tacc_f128_is_zero(a)) {
        tacc_f128_zero(dst);
        dst->sign = res_sign;
        return;
    }

    exponent = tacc_f128_unpack(a, &a_significand);
    exponent = exponent - tacc_f128_unpack(b, &b_significand);

    /*
     * this does not reduce to simple division of integers because the decimal
     * point is located at the start.
     */
    tacc_u128_rsh_n(&a_significand, &a_significand, 1);
    tacc_u128_rsh_n(&b_significand, &b_significand, 1);
    for (i = 0; i < 128; i = i + 1) {
        tacc_u128_lsh_n(&quotient_significand, &quotient_significand, 1);
        if (tacc_u128_uge(&a_significand, &b_significand)) {
            /* remainder >= divisor */
            tacc_u128_sub(&a_significand, &a_significand, &b_significand);
            tacc_u128_or_u32(&quotient_significand, &quotient_significand, 1);
        }
        tacc_u128_lsh_n(&a_significand, &a_significand, 1);
    }
    if (!tacc_u128_is_zero(&a_significand)) {
        /* nonzero remainder; consider this for rounding */
        tacc_u128_or_u32(&quotient_significand, &quotient_significand, 1);
    }
    if ((quotient_significand.a >> 31) == 0) {
        exponent = exponent - 1;
        tacc_u128_lsh_n(&quotient_significand, &quotient_significand, 1);
    }
    sign = 0;
    if (a->sign != b->sign) {
        sign = 1;
    }
    tacc_f128_pack(dst, &quotient_significand, exponent, sign);
}

void tacc_f128_addl_u32(struct tacc_f128 *dst,
                        struct tacc_f128 *a,
                        uint32_t b) {
    struct tacc_f128 aux;
    tacc_f128_from_u32(&aux, b);
    tacc_f128_addl(dst, a, &aux);
}

void tacc_f128_subl_u32(struct tacc_f128 *dst,
                        struct tacc_f128 *a,
                        uint32_t b) {
    struct tacc_f128 aux;
    tacc_f128_from_u32(&aux, b);
    tacc_f128_subl(dst, a, &aux);
}

void tacc_f128_mull_u32(struct tacc_f128 *dst,
                        struct tacc_f128 *a,
                        uint32_t b) {
    struct tacc_f128 aux;
    tacc_f128_from_u32(&aux, b);
    tacc_f128_mull(dst, a, &aux);
}

void tacc_f128_divl_u32(struct tacc_f128 *dst,
                        struct tacc_f128 *a,
                        uint32_t b) {
    struct tacc_f128 aux;
    tacc_f128_from_u32(&aux, b);
    tacc_f128_divl(dst, a, &aux);
}

void tacc_f128_absl(struct tacc_f128 *dst, struct tacc_f128 *src) {
    tacc_f128_copy(dst, src);
    dst->sign = 0;
}

void tacc_f128_copysignl(struct tacc_f128 *dst,
                         struct tacc_f128 *orig,
                         struct tacc_f128 *sign_src) {
    tacc_bool sign;

    sign = sign_src->sign;
    tacc_f128_copy(dst, orig);
    dst->sign = sign;
}

void tacc_f128_fmodl_p2(struct tacc_f128 *dst,
                        struct tacc_f128 *dividend,
                        int modulus_p2) {
    int exp;
    int exp_delta;
    int clz;
    struct tacc_u128 significand;

    if ((modulus_p2 + EXP_BIAS + SIGNIFICANT_DIGITS <= 0) ||
        dividend->exponent == INF_EXPONENT) {
        dst->exponent = INF_EXPONENT;
        dst->mant_a = 0x80000000;
        dst->mant_b = 0;
        dst->mant_c = 0;
        dst->mant_d = 0;
        dst->sign = 0;
        return;
    }
    if (tacc_f128_is_zero(dividend) || (modulus_p2 > MAX_NORM_EXP)) {
        tacc_f128_copy(dst, dividend);
        return;
    }

    exp = tacc_f128_unpack(dividend, &significand);
    if (exp < modulus_p2) {
        /* abs(N) completely within modulus */
        tacc_f128_copy(dst, dividend);
        return;
    }
    if (exp - SIGNIFICANT_DIGITS > modulus_p2) {
        /* we are a multiple of the modulus */
        dst->exponent = 0;
        dst->mant_a = 0;
        dst->mant_b = 0;
        dst->mant_c = 0;
        dst->mant_d = 0;
        dst->sign = dividend->sign;
        return;
    }
    /* TODO: signs correct? */
    exp_delta = modulus_p2 - (exp - SIGNIFICANT_DIGITS);
    tacc_u128_lsh_n(&significand, &significand, exp_delta);
    clz = tacc_u128_clz(&significand);
    tacc_u128_lsh_n(&significand, &significand, clz);

    tacc_f128_pack(dst, &significand, exp - exp_delta - clz, dividend->sign);
}

/* rounds to nearest, ties to even */
void tacc_f128_round_f64(struct tacc_f128 *dst, struct tacc_f128 *src) {
    tacc_bool is_nan;
    tacc_bool sign;
    /* most significant bit that was discarded, a.k.a, "guard" */
    tacc_bool high_discard;
    /*
     * whether we have discarded any bits that are not accounted for by
     * high_discard, a.k.a, "sticky"
     */
    tacc_bool discarded_non_msb;
    uint32_t fully_resident_bits;
    uint32_t mant_mask_keep;
    uint32_t mant_mask_discard;
    uint32_t mant_mask_discard_nonmsb;
    uint32_t mant_mask_discard_msb;
    uint32_t mant_b_lsb;
    uint32_t mant_a_lsb;

    is_nan = tacc_f128_is_nan(src);
    sign = src->sign;

    if (is_nan) {
        return;
    }
    if (src->exponent >= EXP_BIAS + DBL_INF_EXPONENT) {
        /* make into inf, ensure no nan is produced */
        tacc_f128_zero(dst);
        dst->exponent = INF_EXPONENT;
        dst->sign = sign;
        return;
    }
    if (src->exponent < (EXP_BIAS - DBL_BIAS - DBL_SIGNIFICANT_DIGITS)) {
        /*
         * Completely below space of subnormals of f64.
         * Notably this branch includes all subnormals of f128.
         */
        tacc_f128_zero(dst);
        return;
    }

    discarded_non_msb = 128 != 0;
    dst->mant_c = 0;
    dst->mant_b = 0;

    if (src->exponent < (EXP_BIAS - DBL_BIAS)) {
        /* subnormal in f64... */

        /* TODO check */
        fully_resident_bits = (((uint32_t) (src->exponent)) + DBL_BIAS) -
                              EXP_BIAS + DBL_SIGNIFICANT_DIGITS;
    } else {
        /* not subnormal */

        fully_resident_bits = 52;

        /*
         * 20 highest bits of mantissa_b, for a total of 52 bits from mant_a and
         * mant_b combined
         */
    }
    if (fully_resident_bits > 32) {
        mant_mask_keep = 0xffffffff << (32 - (64 - fully_resident_bits));
        mant_mask_discard = ~mant_mask_keep;
        mant_mask_discard_msb = ((uint32_t) 1)
                                << ((32 - (64 - fully_resident_bits)) - 1);
        mant_b_lsb = mant_mask_discard_msb * 2;
        mant_mask_discard_nonmsb = mant_mask_discard ^ mant_mask_discard_msb;
        discarded_non_msb = discarded_non_msb ||
                            ((src->mant_b & mant_mask_discard_nonmsb) != 0);
        high_discard = (src->mant_b & mant_mask_discard_msb) != 0;
        dst->mant_b = dst->mant_b & mant_mask_keep;
    } else {
        mant_b_lsb = 0;
        discarded_non_msb = discarded_non_msb || (src->mant_b != 0);
        src->mant_b = 0;

        /* in case fully resident >= 32, set high_discard */
        high_discard = (src->mant_b >> 31) != 0;
    }
    if (fully_resident_bits >= 32) {
        mant_a_lsb = 1;
        dst->mant_a = dst->mant_a;
    } else {
        mant_mask_keep = 0xffffffff << (32 - fully_resident_bits);
        mant_mask_discard = ~mant_mask_keep;
        mant_mask_discard_msb = ((uint32_t) 1)
                                << (32 - fully_resident_bits - 1);
        mant_a_lsb = mant_mask_discard_msb * 2;
        mant_mask_discard_nonmsb = mant_mask_discard ^ mant_mask_discard_msb;
        discarded_non_msb = discarded_non_msb ||
                            ((src->mant_a & mant_mask_discard_nonmsb) != 0);
        high_discard = (src->mant_a & mant_mask_discard_msb) != 0;
        dst->mant_a = dst->mant_a & mant_mask_keep;
    }

    if (high_discard) {
        if (discarded_non_msb || (dst->mant_b & mant_b_lsb) != 0) {
            /*
             * strictly greater than halfway point between current dst and next
             * f64. round to nearest, that is now, away from zero.
             *
             * OR
             *
             * exact middle between the current dst and and the next f64 away
             * from zero, AND the lower number has lsb=1. the tie must be broken
             * by rounding to next f64, which may end up at infinity.
             *
             * EITHER WAY,
             * we round upwards
             */
            dst->mant_b = dst->mant_b + mant_b_lsb;
            if (dst->mant_b == 0) {
                /* carry from mant_b, or mant_b cleared by subnormality */

                dst->mant_a = dst->mant_a + mant_a_lsb;
                if (dst->mant_a == 0) {
                    /* carry from mant_a; mant_b==mant_a==0 */

                    /* dst can never be inf here */
                    dst->exponent = (uint16_t) (dst->exponent + 1);
                }
            }
        } else {
            /*
             * already lsb=0 which is the corrent tiebreak; nothing to do
             */
        }
    } else {
        /*
         * highest bit discarded was not set. therefore, we are already at
         * the uniquely closest rounding (rounded down)
         */
    }
}

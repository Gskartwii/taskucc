#include "soft_float.h"
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

    if (n == 0) {
        tacc_f128_zero(f);
        return;
    }

    f->sign = 0;

    leading_zeroes = 0;
    for (i = 32; i > 0; i = i - 1) {
        if (((n >> i) & 1) != 0) {
            break;
        }
        leading_zeroes = leading_zeroes + 1;
    }

    f->mant_a = n << (leading_zeroes + 1);
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
                a->mant_a = ((unsigned) 1) << 31;
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

    tacc_u128_from_limbs(&far_f_significand,
                         far_f->mant_a,
                         far_f->mant_b,
                         far_f->mant_c,
                         far_f->mant_d);
    tacc_u128_from_limbs(&near_f_significand,
                         near_f->mant_a,
                         near_f->mant_b,
                         near_f->mant_c,
                         near_f->mant_d);
    if (near_f->exponent == 0) {
        /* nonzero subnormal; shift to normalize */
        near_exponent_adjusted = -tacc_u128_clz(&near_f_significand);
        tacc_u128_lsh_n(
            &near_f_significand, &near_f_significand, -near_exponent_adjusted);
    } else {
        near_exponent_adjusted = ((int) (near_f->exponent)) - EXP_BIAS;

        /* set implicit bit */
        tacc_u128_rsh_n(&near_f_significand, &near_f_significand, 1);
        near_f_significand.a = near_f_significand.a | 0x80000000;
    }
    if (far_f->exponent == 0) {
        far_exponent_adjusted = -tacc_u128_clz(&far_f_significand);
        tacc_u128_lsh_n(
            &far_f_significand, &far_f_significand, -far_exponent_adjusted);
    } else {
        far_exponent_adjusted = ((int) (far_f->exponent)) - EXP_BIAS;

        /* set implicit bit */
        tacc_u128_rsh_n(&far_f_significand, &far_f_significand, 1);
        far_f_significand.a = far_f_significand.a | 0x80000000;
    }
    /*
     * Invariants:
     * - near_f_significand.a & (1<<31) is set, and the same holds for far.
     * - At most 113 MSBs of significands are set.
     */

    /*
     * Align significands such that guard, round and sticky are easily
     * accessible at low 3 bits, and to contain overflow.
     */
    tacc_u128_rsh_n(&near_f_significand, &near_f_significand, 128 - 113 - 3);
    tacc_u128_rsh_n(&far_f_significand, &far_f_significand, 128 - 113 - 3);

    exponent_delta = far_exponent_adjusted - near_exponent_adjusted;
    if (exponent_delta >= 128) {
        /* underflow, set sticky and reset rest of bits */
        tacc_u128_from_limbs(&near_f_significand, 0, 0, 0, 1);
    } else {
        /* save the bits that will be lost... */
        tacc_u128_lsh_n(&u128_aux, &near_f_significand, 128 - exponent_delta);
        tacc_u128_rsh_n(
            &near_f_significand, &near_f_significand, exponent_delta);

        /* lost bits? */
        if (!tacc_u128_is_zero(&u128_aux)) {
            /* ensure sticky bit is set to indicate underflow */
            tacc_u128_or_u32(&near_f_significand, &near_f_significand, 1);
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
        tacc_u128_add(&u128_aux, &far_f_significand, &near_f_significand);
    }

    if ((u128_aux.d & 7) > 4) {
        /* closer than halfway, round up */
        tacc_u128_add_u32(&u128_aux, &u128_aux, 8);
    } else if (((u128_aux.d & 7) == 4) && ((u128_aux.d & 8) != 0)) {
        /* exactly halfway and final digit is odd; round up */
        tacc_u128_add_u32(&u128_aux, &u128_aux, 8);
    }

    exponent_adjust = tacc_u128_clz(&u128_aux);
    final_exponent = (exponent_adjust - 12) + far_exponent_adjusted;

    if (final_exponent + EXP_BIAS < 0) {
        /* subnormal, shift left the best we can */
        tacc_u128_lsh_n(&u128_aux, &u128_aux, 13);
        final_exponent = 0;
    } else {
        /* discard implicit bit */
        tacc_u128_lsh_n(&u128_aux, &u128_aux, exponent_adjust + 1);
    }

    dst->mant_a = u128_aux.a;
    dst->mant_b = u128_aux.b;
    dst->mant_c = u128_aux.c;
    dst->mant_d = u128_aux.d & 0xFFFF0000;
    dst->exponent = (uint16_t) (final_exponent + EXP_BIAS);
    dst->sign = far_f->sign;
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
    TACC_UNUSED(dst);
    TACC_UNUSED(a);
    TACC_UNUSED(b);
    tacc_assert(ASSERT_TODO, 0, "multiply floats");
}

void tacc_f128_divl(struct tacc_f128 *dst,
                    struct tacc_f128 *a,
                    struct tacc_f128 *b) {
    TACC_UNUSED(dst);
    TACC_UNUSED(a);
    TACC_UNUSED(b);
    tacc_assert(ASSERT_TODO, 0, "divide floats");
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

void tacc_f128_fmodl(struct tacc_f128 *dst,
                     struct tacc_f128 *dividend,
                     struct tacc_f128 *divisor) {
    TACC_UNUSED(dst);
    TACC_UNUSED(dividend);
    TACC_UNUSED(divisor);
    tacc_assert(ASSERT_TODO, 0, "fmod floats");
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

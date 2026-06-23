#include "soft_u64.h"

/* M2 unconditionally sign-extends 0xffffffff on x86_64, but might store x in a
 * zero-extended register. So a naive comparison doesn't work. */
#define IS_U32_MAX(x)                                                \
    ((((x) & 0x7FFFFFFF) == 0x7FFFFFFF) && ((((x) >> 31) & 1) == 1))

void tacc_u64_zero(struct tacc_u64 *dst) {
    dst->high = 0;
    dst->low = 0;
}
void tacc_u64_copy(struct tacc_u64 *dst, struct tacc_u64 *src) {
    dst->high = src->high;
    dst->low = src->low;
}
uint32_t tacc_u64_neg(struct tacc_u64 *dst, struct tacc_u64 *src) {
    /* Hacker's Delight 2-2 (a) */
    dst->high = ~(src->high);
    dst->low = ~(src->low);
    if (IS_U32_MAX(dst->low)) {
        dst->high = dst->high + 1;
    }
    dst->low = dst->low + 1;
    return dst->high >> 31;
}
uint32_t tacc_u64_add(struct tacc_u64 *to,
                      struct tacc_u64 *left,
                      struct tacc_u64 *right) {
    /* Hacker's Delight 2-16 */
    uint32_t carry;
    uint32_t l_lo;
    uint32_t r_lo;
    uint32_t l_hi;
    uint32_t r_hi;

    l_lo = left->low;
    r_lo = right->low;
    l_hi = left->high;
    r_hi = right->high;

    to->low = l_lo + r_lo;
    carry = l_lo < to->low;
    to->high = l_hi + r_hi + carry;
    if (l_hi + r_hi < l_hi) {
        return 1;
    }
    if ((IS_U32_MAX(l_hi + r_hi))) {
        return carry;
    }
    return 0;
}
uint32_t tacc_u64_sub(struct tacc_u64 *to,
                      struct tacc_u64 *left,
                      struct tacc_u64 *right) {
    /* Hacker's Delight 2-16 */
    uint32_t carry;
    uint32_t l_lo;
    uint32_t r_lo;
    uint32_t l_hi;
    uint32_t r_hi;

    l_lo = left->low;
    r_lo = right->low;
    l_hi = left->high;
    r_hi = right->high;

    to->low = l_lo - r_lo;
    carry = (uint32_t) (to->low > l_lo);
    to->high = l_hi - r_hi - carry;
    if (l_hi - r_hi < l_hi) {
        return 1;
    }
    if (l_hi == r_hi) {
        return carry;
    }
    return 0;
}
void tacc_u64_mul(struct tacc_u64 *to,
                  struct tacc_u64 *left,
                  struct tacc_u64 *right) {
    uint32_t low = left->low * right->low;
    to->high = (left->low >> 16) * (right->low >> 16) +
               left->high * right->low + left->low * right->high;
    to->low = low;
}

void tacc_u64_lsh_n(struct tacc_u64 *to, struct tacc_u64 *left, int n) {
    int count;
    uint32_t high;
    uint32_t low;

    count = n & 63;
    high = left->high;
    low = left->low;
    if (count == 0) {
        to->high = left->high;
        to->low = left->low;
    } else if (count > 31) {
        to->low = 0;
        to->high = low << (count - 32);
    } else {
        to->low = low << count;
        to->high = (high << count) | (low >> (32 - count));
    }
}
void tacc_u64_lsh(struct tacc_u64 *to,
                  struct tacc_u64 *left,
                  struct tacc_u64 *right) {
    tacc_u64_lsh_n(to, left, (int) (right->low & 63));
}
void tacc_u64_rsh_n(struct tacc_u64 *to, struct tacc_u64 *left, int n) {
    int count;
    uint32_t high;
    uint32_t low;

    count = n & 63;
    high = left->high;
    low = left->low;
    if (count > 31) {
        to->high = 0;
        to->low = high >> (count - 32);
    } else {
        to->high = high >> count;
        to->low = (low >> count) | (high << (32 - count));
    }
}
void tacc_u64_rsh(struct tacc_u64 *to,
                  struct tacc_u64 *left,
                  struct tacc_u64 *right) {
    tacc_u64_rsh_n(to, left, (int) (right->low & 63));
}
void tacc_u64_arsh_n(struct tacc_u64 *to, struct tacc_u64 *left, int n) {
    int count;
    uint32_t high;
    uint32_t low;

    count = n & 63;
    high = left->high;
    low = left->low;
    if (count > 31) {
        to->high = -(high >> 31);
        to->low = high >> (count - 32);
    } else {
        to->high = (high >> count) | ((-(high >> 31)) << (32 - count));
        to->low = (low >> count) | (high << (32 - count));
    }
}
void tacc_u64_arsh(struct tacc_u64 *to,
                   struct tacc_u64 *left,
                   struct tacc_u64 *right) {
    tacc_u64_arsh_n(to, left, (int) (right->low & 63));
}

tacc_bool tacc_u64_eq(struct tacc_u64 *left, struct tacc_u64 *right) {
    return (left->high == right->high) && (left->low == right->low);
}
tacc_bool tacc_u64_ne(struct tacc_u64 *left, struct tacc_u64 *right) {
    return !tacc_u64_eq(left, right);
}
tacc_bool tacc_u64_ult(struct tacc_u64 *left, struct tacc_u64 *right) {
    return left->high < right->high ||
           (left->high == right->high && left->low < right->low);
}
tacc_bool tacc_u64_ugt(struct tacc_u64 *left, struct tacc_u64 *right) {
    return left->high > right->high ||
           (left->high == right->high && left->low > right->low);
}
tacc_bool tacc_u64_ule(struct tacc_u64 *left, struct tacc_u64 *right) {
    return left->high < right->high ||
           (left->high == right->high && left->low <= right->low);
}
tacc_bool tacc_u64_uge(struct tacc_u64 *left, struct tacc_u64 *right) {
    return left->high > right->high ||
           (left->high == right->high && left->low >= right->low);
}
tacc_bool tacc_u64_slt(struct tacc_u64 *left, struct tacc_u64 *right) {
    int32_t l_hi = (int32_t) (left->high);
    int32_t r_hi = (int32_t) (right->high);
    int32_t l_lo = (int32_t) (left->low);
    int32_t r_lo = (int32_t) (right->low);

    return l_hi < r_hi || (l_hi == r_hi && l_lo < r_lo);
}
tacc_bool tacc_u64_sgt(struct tacc_u64 *left, struct tacc_u64 *right) {
    int32_t l_hi = (int32_t) (left->high);
    int32_t r_hi = (int32_t) (right->high);
    int32_t l_lo = (int32_t) (left->low);
    int32_t r_lo = (int32_t) (right->low);

    return l_hi > r_hi || (l_hi == r_hi && l_lo > r_lo);
}
tacc_bool tacc_u64_sle(struct tacc_u64 *left, struct tacc_u64 *right) {
    int32_t l_hi = (int32_t) (left->high);
    int32_t r_hi = (int32_t) (right->high);
    int32_t l_lo = (int32_t) (left->low);
    int32_t r_lo = (int32_t) (right->low);

    return l_hi < r_hi || (l_hi == r_hi && l_lo <= r_lo);
}
tacc_bool tacc_u64_sge(struct tacc_u64 *left, struct tacc_u64 *right) {
    int32_t l_hi = (int32_t) (left->high);
    int32_t r_hi = (int32_t) (right->high);
    int32_t l_lo = (int32_t) (left->low);
    int32_t r_lo = (int32_t) (right->low);

    return l_hi < r_hi || (l_hi == r_hi && l_lo >= r_lo);
}

tacc_bool tacc_u64_sign(struct tacc_u64 *src) { return (src->high >> 31) != 0; }

void tacc_u64_udiv(struct tacc_u64 *quot,
                   struct tacc_u64 *rem,
                   struct tacc_u64 *dividend,
                   struct tacc_u64 *divisor) {
    int i;
    int clz;
    uint32_t clz_src;
    struct tacc_u64 divisor_aux;
    struct tacc_u64 rem_aux;
    struct tacc_u64 quot_aux;

    clz = 0;
    clz_src = divisor->high;
    if (clz_src == 0) {
        clz = 32;
        clz_src = divisor->low;
    }
    for (i = 0; i < 32; i = i + 1) {
        if ((clz_src >> 31) == 1) {
            break;
        }
        clz_src = clz_src << 1;
        clz = clz + 1;
    }

    tacc_u64_copy(&rem_aux, dividend);
    tacc_u64_zero(&quot_aux);
    for (i = 0; i < clz; i = i + 1) {
        tacc_u64_lsh_n(&divisor_aux, divisor, clz - i);
        if (tacc_u64_uge(&rem_aux, &divisor_aux)) {
            /* remainder >= aux * 2**(63-i) */
            tacc_u64_sub(&rem_aux, &rem_aux, &divisor_aux);
            if (clz - i > 32) {
                quot_aux.high =
                    quot_aux.high | (((uint32_t) 1) << (clz - i - 32));
            } else {
                quot_aux.low = quot_aux.low | (((uint32_t) 1) << (clz - i));
            }
        }
    }
    tacc_u64_copy(rem, &rem_aux);
    tacc_u64_copy(quot, &quot_aux);
}

extern int printf(const char *, ...);

void tacc_u64_sdiv(struct tacc_u64 *quot,
                   struct tacc_u64 *rem,
                   struct tacc_u64 *dividend,
                   struct tacc_u64 *divisor) {
    tacc_bool result_negative =
        tacc_u64_sign(dividend) != tacc_u64_sign(divisor);
    if (tacc_u64_sign(dividend)) {
        tacc_u64_neg(dividend, dividend);
    }
    if (tacc_u64_sign(divisor)) {
        tacc_u64_neg(divisor, divisor);
    }
    tacc_u64_udiv(quot, rem, dividend, divisor);
    if (result_negative) {
        tacc_u64_neg(quot, quot);
        tacc_u64_neg(rem, rem);
    }
}

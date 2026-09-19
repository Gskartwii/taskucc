#include "soft_u128.h"
#include "util.h"

void tacc_u128_from_limbs(
    struct tacc_u128 *n, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    n->a = a;
    n->b = b;
    n->c = c;
    n->d = d;
}

tacc_bool tacc_u128_is_zero(struct tacc_u128 *n) {
    return (n->a == 0) && (n->b == 0) && (n->c == 0) && (n->d == 0);
}

void tacc_u128_copy(struct tacc_u128 *dst, struct tacc_u128 *src) {
    dst->a = src->a;
    dst->b = src->b;
    dst->c = src->c;
    dst->d = src->d;
}

static int tacc_clz(uint32_t n) {
    int i;

    for (i = 0; i < 32; i = i + 1) {
        if ((n >> ((unsigned) (31 - i)) & 1) != 0) {
            return i;
        }
    }
    return 32;
}

int tacc_u128_clz(struct tacc_u128 *n) {
    if (n->a != 0) {
        return tacc_clz(n->a);
    }
    if (n->b != 0) {
        return 32 + tacc_clz(n->b);
    }
    if (n->c != 0) {
        return 64 + tacc_clz(n->c);
    }
    return 96 + tacc_clz(n->d);
}

static uint32_t tacc_rsh_or_trunc(uint32_t in, unsigned int n) {
    if (n == 32) {
        return 0;
    } else {
        return in >> n;
    }
}

static uint32_t tacc_lsh_or_trunc(uint32_t in, unsigned int n) {
    if (n == 32) {
        return 0;
    } else {
        return in << n;
    }
}

void tacc_u128_lsh_n(struct tacc_u128 *dst, struct tacc_u128 *src, int n) {
    unsigned count;

    count = n & 127;

    if (count >= 96) {
        count = count - 96;
        dst->a = src->d << count;
        dst->b = 0;
        dst->c = 0;
        dst->d = 0;
        return;
    }
    if (count >= 64) {
        count = count - 64;
        dst->a = src->c << count | tacc_rsh_or_trunc(dst->d, 32 - count);
        dst->b = src->d << count;
        dst->c = 0;
        dst->d = 0;
        return;
    }
    if (count >= 32) {
        count = count - 32;
        dst->a = src->b << count | tacc_rsh_or_trunc(dst->c, 32 - count);
        dst->b = src->c << count | tacc_rsh_or_trunc(dst->d, 32 - count);
        dst->c = src->d << count;
        dst->d = 0;
        return;
    }
    dst->a = src->a << count | tacc_rsh_or_trunc(dst->b, 32 - count);
    dst->b = src->b << count | tacc_rsh_or_trunc(dst->c, 32 - count);
    dst->c = src->c << count | tacc_rsh_or_trunc(dst->d, 32 - count);
    dst->d = src->d << count;
}

void tacc_u128_rsh_n(struct tacc_u128 *dst, struct tacc_u128 *src, int n) {
    unsigned count;

    count = n & 127;

    if (count >= 96) {
        count = count - 96;
        dst->d = src->a >> count;
        dst->c = 0;
        dst->b = 0;
        dst->a = 0;
        return;
    }
    if (count >= 64) {
        count = count - 64;
        dst->d = src->b >> count | tacc_lsh_or_trunc(dst->a, 32 - count);
        dst->c = src->a >> count;
        dst->b = 0;
        dst->a = 0;
        return;
    }
    if (count >= 32) {
        count = count - 32;
        dst->d = src->c >> count | tacc_lsh_or_trunc(dst->b, 32 - count);
        dst->c = src->b >> count | tacc_lsh_or_trunc(dst->a, 32 - count);
        dst->b = src->a >> count;
        dst->a = 0;
        return;
    }
    dst->d = src->d >> count | tacc_lsh_or_trunc(dst->c, 32 - count);
    dst->c = src->c >> count | tacc_lsh_or_trunc(dst->b, 32 - count);
    dst->b = src->b >> count | tacc_lsh_or_trunc(dst->a, 32 - count);
    dst->a = src->a >> count;
}

void tacc_u128_or_u32(struct tacc_u128 *dst, struct tacc_u128 *a, uint32_t b) {
    tacc_u128_copy(dst, a);
    dst->d = dst->d | b;
}

tacc_bool tacc_u128_add(struct tacc_u128 *dst,
                        struct tacc_u128 *a,
                        struct tacc_u128 *b) {
    struct tacc_u128 aux;
    tacc_bool carry;

    carry = 0;

    aux.a = a->a + b->a;
    if (aux.a < a->a) {
        carry = 1;
    }

    aux.b = a->b + b->b;
    if (aux.b < a->b) {
        /* carry */
        aux.a = aux.a + 1;
        if (aux.a == 0) {
            carry = 1;
        }
    }

    aux.c = a->c + b->c;
    if (aux.c < a->c) {
        aux.b = aux.b + 1;
        if (aux.b == 0) {
            aux.a = aux.a + 1;
            if (aux.a == 0) {
                carry = 1;
            }
        }
    }

    aux.d = a->d + b->d;
    if (aux.d < a->d) {
        aux.c = aux.c + 1;
        if (aux.c == 0) {
            aux.b = aux.b + 1;
            if (aux.b == 0) {
                aux.a = aux.a + 1;
                if (aux.a == 0) {
                    carry = 1;
                }
            }
        }
    }
    tacc_u128_copy(dst, &aux);

    return carry;
}

void tacc_u128_add_u32(struct tacc_u128 *dst, struct tacc_u128 *a, uint32_t b) {
    struct tacc_u128 aux;

    tacc_u128_from_limbs(&aux, 0, 0, 0, b);
    tacc_u128_add(dst, a, &aux);
}

static uint32_t tacc_u128_add_u32_at_limb(struct tacc_u128 *dst,
                                          struct tacc_u128 *a,
                                          uint32_t b,
                                          int limb) {
    struct tacc_u128 aux;

    switch (limb) {
    case 0:
        tacc_u128_from_limbs(&aux, 0, 0, 0, b);
        break;
    case 1:
        tacc_u128_from_limbs(&aux, 0, 0, b, 0);
        break;
    case 2:
        tacc_u128_from_limbs(&aux, 0, b, 0, 0);
        break;
    case 3:
        tacc_u128_from_limbs(&aux, b, 0, 0, 0);
        break;
    default:
        tacc_assert(ASSERT_TODO, 0, "invalid limb %d", limb);
        return 0;
    }
    return (uint32_t) tacc_u128_add(dst, a, &aux);
}

void tacc_u128_sub(struct tacc_u128 *dst,
                   struct tacc_u128 *a,
                   struct tacc_u128 *b) {
    struct tacc_u128 aux;

    aux.a = a->a - b->a;
    aux.b = a->b - b->b;
    if (aux.b > a->b) {
        /* carry */
        aux.a = aux.a - 1;
    }

    aux.c = a->c - b->c;
    if (aux.c > a->c) {
        aux.b = aux.b - 1;
        if (IS_U32_MAX(aux.b)) {
            aux.a = aux.a - 1;
        }
    }

    aux.d = a->d - b->d;
    if (aux.d > a->d) {
        aux.c = aux.c - 1;
        if (IS_U32_MAX(aux.c)) {
            aux.b = aux.b - 1;
            if (IS_U32_MAX(aux.b)) {
                aux.a = aux.a - 1;
            }
        }
    }
    tacc_u128_copy(dst, &aux);
}

void tacc_u128_mul_widening(struct tacc_u128 *dst_high,
                            struct tacc_u128 *dst_low,
                            struct tacc_u128 *a,
                            struct tacc_u128 *b) {
    /*
     * - 0 := 0*0
     *   producing no big carry
     * - 16 := 16 * 0 + 0 * 16
     *   all producing big carry to 32
     * - 32 := 32*0 + 0*32 + 16*16
     *   producing no big carry
     * - 48 := 48*0 + 0*48 + 16*32 + 32*16
     *   all producing big carry to 64
     * - 64 := 64*0 + 0*64 + 48*16 + 16*48 + 32*32
     *   producing no big carry
     * - 80 := 80*0 + 0*80 + 64*16 + 16*64 + 48*32 + 32*48
     *   all producing big carry to 96
     * - 96 := 96*0 * 0*96 + 80*16 + 16*80 + 64*32 + 32*64 + 48*48
     *   producing no big carry
     * - 112 := 112*0 + 0*112 + 96*16 + 16*96 + 80*32 + 32*80 + 48*64 + 64*48
     *   all producing big carry to 128
     * - 128 := 112*16 + 16*112 + 96*32 + 32*96 + 80*48 + 48*80 + 64*64
     *   producing no big carry
     * - 144 := 112*32 + 32*112 + 96*48 + 48*96 + 80*64 + 64*80
     *   all producing big carry to 160
     * - 160 := 112*48 + 48*112 + 96*64 + 64*96 + 80*80
     *   producing no big carry
     * - 176 := 112*64 + 64*112 + 96*80 + 80*96
     *   all producing big carry to 192
     * - 192 := 112*80 + 80*112 + 96*96
     *   producing no big carry
     * - 208 := 112*96 + 96*112
     *   all producing big carry to 224
     * - 224 := 112*112
     *   producing no big carry
     */

    uint32_t a_0;
    uint32_t a_16;
    uint32_t a_32;
    uint32_t a_48;
    uint32_t a_64;
    uint32_t a_80;
    uint32_t a_96;
    uint32_t a_112;
    uint32_t b_0;
    uint32_t b_16;
    uint32_t b_32;
    uint32_t b_48;
    uint32_t b_64;
    uint32_t b_80;
    uint32_t b_96;
    uint32_t b_112;
    uint32_t c;

    a_0 = a->d & 0xFFFF;
    a_16 = (a->d >> 16) & 0xFFFF;
    a_32 = a->c & 0xFFFF;
    a_48 = (a->c >> 16) & 0xFFFF;
    a_64 = a->b & 0xFFFF;
    a_80 = (a->b >> 16) & 0xFFFF;
    a_96 = a->a & 0xFFFF;
    a_112 = (a->a >> 16) & 0xFFFF;
    b_0 = b->d & 0xFFFF;
    b_16 = (b->d >> 16) & 0xFFFF;
    b_32 = b->c & 0xFFFF;
    b_48 = (b->c >> 16) & 0xFFFF;
    b_64 = b->b & 0xFFFF;
    b_80 = (b->b >> 16) & 0xFFFF;
    b_96 = b->a & 0xFFFF;
    b_112 = (b->a >> 16) & 0xFFFF;

    tacc_u128_from_limbs(dst_high, 0, 0, 0, 0);
    tacc_u128_from_limbs(dst_low, 0, 0, 0, 0);

    /* 0 := 0*0 */
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_0 * b_0, 0);

    /* 16 := 16 * 0 + 0 * 16 */
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_0 * b_16) << 16, 0);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_16 * b_0) << 16, 0);
    /* all producing big carry to 32 */
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_0 * b_16) >> 16, 1);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_16 * b_0) >> 16, 1);

    /* 32 := 32*0 + 0*32 + 16*16 */
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_32 * b_0, 1);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_0 * b_32, 1);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_16 * b_16, 1);

    /* 48 := 48*0 + 0*48 + 16*32 + 32*16 */
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_48 * b_0) << 16, 1);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_0 * b_48) << 16, 1);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_16 * b_32) << 16, 1);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_32 * b_16) << 16, 1);
    /* all producing big carry to 64 */
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_48 * b_0) >> 16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_0 * b_48) >> 16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_16 * b_32) >> 16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_32 * b_16) >> 16, 2);

    /* 64 := 64*0 + 0*64 + 48*16 + 16*48 + 32*32 */
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_64 * b_0, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_0 * b_64, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_48 * b_16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_16 * b_48, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, a_32 * b_32, 2);

    /* 80 := 80*0 + 0*80 + 64*16 + 16*64 + 48*32 + 32*48 */
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_80 * b_0) << 16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_0 * b_80) << 16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_64 * b_16) << 16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_16 * b_64) << 16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_48 * b_32) << 16, 2);
    tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_32 * b_48) << 16, 2);
    /* all producing big carry to 96 */
    c = tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_80 * b_0) >> 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_0 * b_80) >> 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_64 * b_16) >> 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_16 * b_64) >> 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_48 * b_32) >> 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_32 * b_48) >> 16, 3);

    /* 96 := 96*0 * 0*96 + 80*16 + 16*80 + 64*32 + 32*64 + 48*48 */
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, a_96 * b_0, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, a_0 * b_96, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, a_80 * b_16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, a_16 * b_80, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, a_64 * b_32, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, a_32 * b_64, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, a_48 * b_48, 3);

    /* 112 := 112*0 + 0*112 + 96*16 + 16*96 + 80*32 + 32*80 + 48*64 + 64*48 */
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_112 * b_0) << 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_0 * b_112) << 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_96 * b_16) << 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_16 * b_96) << 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_80 * b_32) << 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_32 * b_80) << 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_64 * b_48) << 16, 3);
    c = c + tacc_u128_add_u32_at_limb(dst_low, dst_low, (a_48 * b_64) << 16, 3);

    /* c now contains final mini carry from low to high, insert */
    tacc_u128_from_limbs(dst_high, 0, 0, 0, c);

    /*
     * 112 := 112*0 + 0*112 + 96*16 + 16*96 + 80*32 + 32*80 + 48*64 + 64*48
     * produces big carry
     */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_112 * b_0) >> 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_0 * b_112) >> 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_96 * b_16) >> 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_16 * b_96) >> 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_80 * b_32) >> 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_32 * b_80) >> 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_64 * b_48) >> 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_48 * b_64) >> 16, 0);

    /* 128 := 112*16 + 16*112 + 96*32 + 32*96 + 80*48 + 48*80 + 64*64 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_112 * b_16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_16 * b_112, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_96 * b_32, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_32 * b_96, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_80 * b_48, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_48 * b_80, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_64 * b_64, 0);

    /* 144 := 112*32 + 32*112 + 96*48 + 48*96 + 80*64 + 64*80 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_112 * b_32) << 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_32 * b_112) << 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_96 * b_48) << 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_48 * b_96) << 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_80 * b_64) << 16, 0);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_64 * b_80) << 16, 0);
    /* all producing big carry to 160 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_112 * b_32) >> 16, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_32 * b_112) >> 16, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_96 * b_48) >> 16, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_48 * b_96) >> 16, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_80 * b_64) >> 16, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_64 * b_80) >> 16, 1);

    /* 160 := 112*48 + 48*112 + 96*64 + 64*96 + 80*80 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_112 * b_48, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_48 * b_112, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_96 * b_64, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_64 * b_96, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_80 * b_80, 1);

    /* 176 := 112*64 + 64*112 + 96*80 + 80*96 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_112 * b_64) << 16, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_64 * b_112) << 16, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_96 * b_80) << 16, 1);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_80 * b_96) << 16, 1);
    /* all producing big carry to 192 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_112 * b_64) >> 16, 2);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_64 * b_112) >> 16, 2);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_96 * b_80) >> 16, 2);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_80 * b_96) >> 16, 2);

    /* 192 := 112*80 + 80*112 + 96*96 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_112 * b_80, 2);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_80 * b_112, 2);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_96 * b_96, 2);

    /* 208 := 112*96 + 96*112 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_112 * b_96) << 16, 2);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_96 * b_112) << 16, 2);
    /* all producing big carry to 224 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_112 * b_96) >> 16, 3);
    tacc_u128_add_u32_at_limb(dst_high, dst_high, (a_96 * b_112) >> 16, 3);

    /* 224 := 112*112 */
    tacc_u128_add_u32_at_limb(dst_high, dst_high, a_112 * b_112, 3);
}

tacc_bool tacc_u128_uge(struct tacc_u128 *left, struct tacc_u128 *right) {
    if (left->a > right->a) {
        return 1;
    }
    if (left->a < right->a) {
        return 0;
    }
    if (left->b > right->b) {
        return 1;
    }
    if (left->b < right->b) {
        return 0;
    }
    if (left->c > right->c) {
        return 1;
    }
    if (left->c < right->c) {
        return 0;
    }
    if (left->d > right->d) {
        return 1;
    }
    if (left->d < right->d) {
        return 0;
    }
    return 1;
}

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
        return 32 + tacc_clz(n->a);
    }
    if (n->c != 0) {
        return 64 + tacc_clz(n->a);
    }
    return 96 + tacc_clz(n->a);
}

void tacc_u128_lsh_n(struct tacc_u128 *dst, struct tacc_u128 *src, int n) {
    unsigned count;

    count = n & 127;

    if (count > 96) {
        count = count - 96;
        dst->a = src->d << count;
        dst->b = 0;
        dst->c = 0;
        dst->d = 0;
        return;
    }
    if (count > 64) {
        count = count - 64;
        dst->a = src->c << count | (dst->d >> (32 - count));
        dst->b = src->d << count;
        dst->c = 0;
        dst->d = 0;
        return;
    }
    if (count > 32) {
        count = count - 32;
        dst->a = src->b << count | (dst->c >> (32 - count));
        dst->b = src->c << count | (dst->d >> (32 - count));
        dst->c = src->d << count;
        dst->d = 0;
        return;
    }
    dst->a = src->a << count | (dst->b >> (32 - count));
    dst->b = src->b << count | (dst->c >> (32 - count));
    dst->c = src->c << count | (dst->d >> (32 - count));
    dst->d = src->d << count;
}

void tacc_u128_rsh_n(struct tacc_u128 *dst, struct tacc_u128 *src, int n) {
    unsigned count;

    count = n & 127;

    if (count > 96) {
        count = count - 96;
        dst->d = src->a >> count;
        dst->c = 0;
        dst->b = 0;
        dst->a = 0;
        return;
    }
    if (count > 64) {
        count = count - 64;
        dst->d = src->b >> count | (dst->a << (32 - count));
        dst->c = src->a >> count;
        dst->b = 0;
        dst->a = 0;
        return;
    }
    if (count > 32) {
        count = count - 32;
        dst->d = src->c >> count | (dst->b << (32 - count));
        dst->c = src->b >> count | (dst->a << (32 - count));
        dst->b = src->a >> count;
        dst->a = 0;
        return;
    }
    dst->d = src->d >> count | (dst->c << (32 - count));
    dst->c = src->c >> count | (dst->b << (32 - count));
    dst->b = src->b >> count | (dst->a << (32 - count));
    dst->a = src->a >> count;
}

void tacc_u128_or_u32(struct tacc_u128 *dst, struct tacc_u128 *a, uint32_t b) {
    tacc_u128_copy(dst, a);
    dst->d = dst->d | b;
}

void tacc_u128_add(struct tacc_u128 *dst,
                   struct tacc_u128 *a,
                   struct tacc_u128 *b) {
    struct tacc_u128 aux;

    aux.a = a->a + b->a;
    aux.b = a->b + b->b;
    if (aux.b < a->b) {
        /* carry */
        aux.a = aux.a + 1;
    }

    aux.c = a->c + b->c;
    if (aux.c < a->c) {
        aux.b = aux.b + 1;
        if (aux.b == 0) {
            aux.a = aux.a + 1;
        }
    }

    aux.d = a->d + b->d;
    if (aux.d < a->d) {
        aux.c = aux.c + 1;
        if (aux.c == 0) {
            aux.b = aux.b + 1;
            if (aux.b == 0) {
                aux.a = aux.a + 1;
            }
        }
    }
    tacc_u128_copy(dst, &aux);
}

void tacc_u128_add_u32(struct tacc_u128 *dst, struct tacc_u128 *a, uint32_t b) {
    struct tacc_u128 aux;

    tacc_u128_from_limbs(&aux, 0, 0, 0, b);
    tacc_u128_add(dst, a, &aux);
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

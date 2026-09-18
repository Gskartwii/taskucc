#include "3rdparty/floatscan.h"
#include "soft_float.h"
#include "soft_u64.h"
#include "util.h"
#include <stdio.h>

#define ZERO               \
    tacc_u64_zero(&a);     \
    tacc_u64_zero(&b);     \
    tacc_u64_zero(&c);     \
    tacc_u64_zero(&d);     \
    tacc_u64_zero(&exp_x); \
    tacc_u64_zero(&exp_y);

#define ZERO_F128           \
    tacc_f128_zero(&a_f);   \
    tacc_f128_zero(&b_f);   \
    tacc_f128_zero(&c_f);   \
    tacc_f128_zero(&exp_f);

#define READ4                           \
    data = read_test_val(data, &a);     \
    data = read_test_val(data, &b);     \
    data = read_test_val(data, &exp_x); \
    data = read_test_val(data, &exp_y);

#define READ_F128(x_f) data = read_test_val_f128(data, &x_f)

#define READ3_F128                           \
    data = read_test_val_f128(data, &a_f);   \
    data = read_test_val_f128(data, &b_f);   \
    data = read_test_val_f128(data, &exp_f);

#define PRINT4(suite)                            \
    printf("[%s] %x:%x ~ %x:%x = %x:%x ~ %x:%x", \
           suite,                                \
           a.high,                               \
           a.low,                                \
           b.high,                               \
           b.low,                                \
           exp_x.high,                           \
           exp_x.low,                            \
           exp_y.high,                           \
           exp_y.low)

#define PRINT3_F128(suite)                                                                                   \
    printf(                                                                                                  \
        "[%s] (-1)^%d * %x:%x:%x:%x * 2^%d ~ (-1)^%d * %x:%x:%x:%x * 2^%d = (-1)^%d * %x:%x:%x:%x * 2^%d  ", \
        suite,                                                                                               \
        a_f.sign,                                                                                            \
        a_f.mant_a,                                                                                          \
        a_f.mant_b,                                                                                          \
        a_f.mant_c,                                                                                          \
        a_f.mant_d,                                                                                          \
        a_f.exponent,                                                                                        \
        b_f.sign,                                                                                            \
        b_f.mant_a,                                                                                          \
        b_f.mant_b,                                                                                          \
        b_f.mant_c,                                                                                          \
        b_f.mant_d,                                                                                          \
        b_f.exponent,                                                                                        \
        exp_f.sign,                                                                                          \
        exp_f.mant_a,                                                                                        \
        exp_f.mant_b,                                                                                        \
        exp_f.mant_c,                                                                                        \
        exp_f.mant_d,                                                                                        \
        exp_f.exponent)

#define CHECK2                      \
    ok = ok & check_eq(&c, &exp_x); \
    ok = ok & check_eq(&d, &exp_y); \
    printf("\n");

#define CHECK_F128                         \
    ok = ok & check_eq_f128(&c_f, &exp_f); \
    printf("\n");

/* clang-format off */
uint32_t sdiv_test_data[] = {
    /* 0 ..= 9 */
    0x00000000, 0x00000000, /*,*/ 0x00000000, 0x00000001, /*,*/  0x00000000, 0x00000000, /*,*/ 0x00000000, 0x00000000,
    0x00000000, 0x00000000, /*,*/ 0xFFFFFFFF, 0xFFFFFFFF, /*,*/  0x00000000, 0x00000000, /*,*/ 0x00000000, 0x00000000,
    0x00000000, 0x00000002, /*,*/ 0x00000000, 0x00000001, /*,*/  0x00000000, 0x00000002, /*,*/ 0x00000000, 0x00000000,
    0x00000000, 0x00000002, /*,*/ 0xFFFFFFFF, 0xFFFFFFFF, /*,*/  0xFFFFFFFF, 0xFFFFFFFE, /*,*/ 0x00000000, 0x00000000,
    0xFFFFFFFF, 0xFFFFFFFE, /*,*/ 0x00000000, 0x00000001, /*,*/  0xFFFFFFFF, 0xFFFFFFFE, /*,*/ 0x00000000, 0x00000000,
    0xFFFFFFFF, 0xFFFFFFFE, /*,*/ 0xFFFFFFFF, 0xFFFFFFFF, /*,*/  0x00000000, 0x00000002, /*,*/ 0x00000000, 0x00000000,
    0x80000000, 0x00000000, /*,*/ 0x00000000, 0x00000001, /*,*/  0x80000000, 0x00000000, /*,*/ 0x00000000, 0x00000000,
    0x80000000, 0x00000000, /*,*/ 0xFFFFFFFF, 0xFFFFFFFF, /*,*/  0x80000000, 0x00000000, /*,*/ 0x00000000, 0x00000000,
    0x80000000, 0x00000000, /*,*/ 0xFFFFFFFF, 0xFFFFFFFE, /*,*/  0x40000000, 0x00000000, /*,*/ 0x00000000, 0x00000000,
    0x80000000, 0x00000000, /*,*/ 0x00000000, 0x00000002, /*,*/  0xC0000000, 0x00000000, /*,*/ 0x00000000, 0x00000000,
};

uint32_t f128add_test_data[] = {
    /* 0 + 0 = 0 */     0, 0, 0, 0, /*,*/ 0, 0, 0, 0, /*,*/ 0, 0, 0, 0,
    /* -0 + 0 = 0 */    0x80000000, 0, 0, 0, /*,*/ 0, 0, 0, 0, /*,*/ 0, 0, 0, 0,
    /* -0 + -0 = -0 */  0x80000000, 0, 0, 0, /*,*/ 0x80000000, 0, 0, 0, /*,*/ 0x80000000, 0, 0, 0,
    /* 1 + 2 = 3 */     0x3FFF0000, 0, 0, 0, /*,*/ 0x40000000, 0, 0, 0, /*,*/ 0x40008000, 0, 0, 0,
};
size_t count_f128add_data = 4;

uint32_t fparse_test_data[] = {
    /* 0.0 */ 0, 0, 0, 0,
    /* 1.0 */ 0x3FFF0000, 0, 0, 0,
};
char *fparse_test_cases[] = {
    "0.0",
    "1.0",
};
size_t count_fparse_data = 2;
/* clang-format on */

int check_eq(struct tacc_u64 *a, struct tacc_u64 *exp) {
    if ((a->high != exp->high) || (a->low != exp->low)) {
        printf("  %x:%x != %x:%x (expected)",
               a->high,
               a->low,
               exp->high,
               exp->low);
        return 0;
    }
    return 1;
}

int check_eq_f128(struct tacc_f128 *a, struct tacc_f128 *exp) {
    if ((a->mant_a != exp->mant_a) || (a->mant_b != exp->mant_b) ||
        (a->mant_c != exp->mant_c) || (a->mant_d != exp->mant_d)) {
        printf("  mantissa %x:%x:%x:%x != %x:%x:%x:%x (expected)",
               a->mant_a,
               a->mant_b,
               a->mant_c,
               a->mant_d,
               exp->mant_a,
               exp->mant_b,
               exp->mant_c,
               exp->mant_d);
        return 0;
    }
    if (a->exponent != exp->exponent) {
        printf("  exponent %x != %x (expected)",
               (uint32_t) (a->exponent),
               (uint32_t) (exp->exponent));
        return 0;
    }
    if (a->sign != exp->sign) {
        printf("  sign %x != %x (expected)",
               (uint32_t) (a->sign),
               (uint32_t) (exp->sign));
        return 0;
    }
    return 1;
}

uint32_t *read_test_val(uint32_t *data, struct tacc_u64 *out) {
    char *out_data;
    uint32_t *out_data_x;

    out_data = (char *) data;

    out->high = *data;
    out_data = ((char *) data) + 4;
    out_data_x = (uint32_t *) out_data;
    out->low = *out_data_x;
    out_data = out_data + 4;

    return (uint32_t *) out_data;
}

uint32_t *read_test_val_f128(uint32_t *data, struct tacc_f128 *out) {
    char *out_data;
    uint32_t *out_data_x;
    uint32_t word;

    out_data = (char *) data;

    tacc_f128_zero(out);

    word = *data;
    if ((word >> 31) != 0) {
        out->sign = 1;
    }
    out->exponent = (word >> 16) & 0x7FFF;
    out->mant_a = word << 16;

    out_data = ((char *) data) + 4;
    out_data_x = (uint32_t *) out_data;
    word = *out_data_x;
    out->mant_a = out->mant_a | (word >> ((unsigned) 16));
    out->mant_b = word << 16;

    out_data = out_data + 4;
    out_data_x = (uint32_t *) out_data;
    word = *out_data_x;
    out->mant_b = out->mant_b | (word >> ((unsigned) 16));
    out->mant_c = word << 16;

    out_data = out_data + 4;
    out_data_x = (uint32_t *) out_data;
    word = *out_data_x;
    out->mant_c = out->mant_c | (word >> ((unsigned) 16));
    out->mant_d = word << 16;

    out_data = out_data + 4;

    return (uint32_t *) out_data;
}

int run_tests(void) {
    struct tacc_u64 a;
    struct tacc_u64 b;
    struct tacc_u64 c;
    struct tacc_u64 d;
    struct tacc_u64 exp_x;
    struct tacc_u64 exp_y;
    struct tacc_f128 a_f;
    struct tacc_f128 b_f;
    struct tacc_f128 c_f;
    struct tacc_f128 exp_f;
    struct tacc_file_iter *iter;
    char *fstr;
    char *f_strs;
    char **f_strs_2;

    size_t i;
    tacc_bool ok;
    uint32_t *data;

    data = (uint32_t *) sdiv_test_data;
    ok = 1;
    for (i = 0; i < 10; i = i + 1) {
        ZERO READ4 PRINT4("sdiv");
        tacc_u64_sdiv(&c, &d, &a, &b);
        CHECK2
    }

    data = (uint32_t *) f128add_test_data;
    for (i = 0; i < count_f128add_data; i = i + 1) {
        ZERO_F128 READ3_F128 PRINT3_F128("f128_add");
        tacc_f128_addl(&c_f, &a_f, &b_f);
        CHECK_F128
    }

    data = (uint32_t *) fparse_test_data;
    f_strs = (char *) fparse_test_cases;

    for (i = 0; i < count_fparse_data; i = i + 1) {
        f_strs_2 = (char **) f_strs;
        fstr = *f_strs_2;
        f_strs = f_strs + sizeof(char *);

        ZERO_F128
        printf("[fparse] %s ", fstr);
        iter = tacc_file_iter_new_str(fstr, fstr + strlen(fstr));
        floatscan(iter, 3, &c_f);
        tacc_free(iter);
        READ_F128(exp_f);
        CHECK_F128
    }

    return !ok;
}

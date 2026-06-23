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

#define READ4                           \
    data = read_test_val(data, &a);     \
    data = read_test_val(data, &b);     \
    data = read_test_val(data, &exp_x); \
    data = read_test_val(data, &exp_y);

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

#define CHECK2                      \
    ok = ok & check_eq(&c, &exp_x); \
    ok = ok & check_eq(&d, &exp_y); \
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
/* clang-format on */

int check_eq(struct tacc_u64 *a, struct tacc_u64 *exp) {
    if ((a->high != exp->high) || (a->low != exp->low)) {
        printf("  %x:%x != %x:%x (expected)",
               a->high,
               a->low,
               exp->high,
               exp->low);
        return 1;
    }
    return 0;
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

int run_tests(void) {
    struct tacc_u64 a;
    struct tacc_u64 b;
    struct tacc_u64 c;
    struct tacc_u64 d;
    struct tacc_u64 exp_x;
    struct tacc_u64 exp_y;
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

    return !ok;
}

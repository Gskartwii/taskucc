#include "binops.inc"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#define DECL_BINOP(output, left, right, op, suffix) \
    extern output binop_##suffix(left, right);

extern unsigned long long binop_rsh_di(unsigned long long x, int c);
extern unsigned int binop_rsh_si(unsigned int x, int c);
extern int binop_rsh_hi(unsigned short x, int c);
extern int binop_rsh_qi(unsigned char x, int c);
extern long long binop_srsh_di(long long x, int c);
extern int binop_srsh_si(int x, int c);
extern int binop_srsh_hi(short x, int c);
extern int binop_srsh_qi(char x, int c);
extern unsigned long long binop_lsh_di(unsigned long long x, int c);
extern unsigned int binop_lsh_si(unsigned int x, int c);
extern int binop_lsh_hi(unsigned short x, int c);
extern int binop_lsh_qi(unsigned char x, int c);

#define GENERIC_PRINT_SPEC(expr)    \
    _Generic(expr,                  \
        _Bool: "%d",                \
        unsigned char: "%d",        \
        char: "%d",                 \
        unsigned short: "%d",       \
        short: "%d",                \
        unsigned int: "%u",         \
        int: "%d",                  \
        unsigned long: "%lu",       \
        long: "%ld",                \
        unsigned long long: "%llu", \
        long long: "%lld",          \
        float: "%f",                \
        double: "%lf",              \
        long double: "%Lf")

#define CHECK_BINOP(output, left, right, op, suffix)                  \
    {                                                                 \
        left l = (left) check_n_l;                                    \
        right r = (right) check_n_r;                                  \
        if (!r) {                                                     \
            continue; /* avoid div by zero */                         \
        }                                                             \
        output expected = l op r;                                     \
        if (binop_##suffix(l, r) != expected) {                       \
            printf("binop failed on (" #left ") ");                   \
            printf(GENERIC_PRINT_SPEC(left), l);                      \
            printf(" %s (" #right ") ", #op);                         \
            printf(GENERIC_PRINT_SPEC(right), r);                     \
            printf(" -> " #output ": ");                              \
            printf(GENERIC_PRINT_SPEC(output), binop_##suffix(l, r)); \
            printf(" != ");                                           \
            printf(GENERIC_PRINT_SPEC(output), expected);             \
            printf("\n");                                             \
            ok = 0;                                                   \
        }                                                             \
    }

#define BINOP_ACTION DECL_BINOP
BINOP_LIST
#undef BINOP_ACTION

#define BINOP_ACTION CHECK_BINOP

int main(void) {
    _Bool ok = 1;
    long long check_ns[] = {
        0,       1,       2,        3,        12,        15,
        16,      17,      31,       32,       33,        36,
        63,      64,      65,       -1,       -50,       255,
        128,     -256,    65535,    -32768,   32768,     32767,
        INT_MIN, INT_MAX, LONG_MIN, LONG_MAX, LLONG_MIN, LLONG_MAX,
    };

    for (size_t n_l = 0; n_l < sizeof(check_ns) / sizeof(check_ns[0]); n_l++) {
        for (size_t n_r = 0; n_r < sizeof(check_ns) / sizeof(check_ns[0]);
             n_r++) {
            long long check_n_l = check_ns[n_l];
            long long check_n_r = check_ns[n_r];
            BINOP_LIST
        }
        for (int c = 0; c < 63; c++) {
            long long check_n_l = check_ns[n_l];
            int check_n_r = c;
            if (c < 8) {
                CHECK_BINOP(int, unsigned char, int, <<, lsh_qi)
                CHECK_BINOP(int, unsigned char, int, >>, rsh_qi)
                CHECK_BINOP(int, char, int, >>, srsh_qi)
            }
            if (c < 16) {
                CHECK_BINOP(int, unsigned short, int, <<, lsh_hi)
                CHECK_BINOP(int, unsigned short, int, >>, rsh_hi)
                CHECK_BINOP(int, short, int, >>, srsh_hi)
            }
            if (c < 32) {
                CHECK_BINOP(unsigned int, unsigned int, int, <<, lsh_si)
                CHECK_BINOP(unsigned int, unsigned int, int, >>, rsh_si)
                CHECK_BINOP(int, int, int, >>, srsh_si)
            }
            CHECK_BINOP(unsigned long long, unsigned long long, int, <<, lsh_di)
            CHECK_BINOP(unsigned long long, unsigned long long, int, >>, rsh_di)
            CHECK_BINOP(long long, long long, int, >>, srsh_di)
        }
    }

    return !ok;
}

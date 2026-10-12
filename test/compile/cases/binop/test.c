#include "binops.inc"

#define DEF_BINOP(output, left, right, op, suffix)            \
    output binop_##suffix(left l, right r) { return l op r; }

#define BINOP_ACTION DEF_BINOP
BINOP_LIST

unsigned long long binop_rsh_di(unsigned long long x, int c) { return x >> c; }
unsigned int binop_rsh_si(unsigned int x, int c) { return x >> c; }
int binop_rsh_hi(unsigned short x, int c) { return x >> c; }
int binop_rsh_qi(unsigned char x, int c) { return x >> c; }
long long binop_srsh_di(long long x, int c) { return x >> c; }
int binop_srsh_si(int x, int c) { return x >> c; }
int binop_srsh_hi(short x, int c) { return x >> c; }
int binop_srsh_qi(char x, int c) { return x >> c; }
unsigned long long binop_lsh_di(unsigned long long x, int c) { return x << c; }
unsigned int binop_lsh_si(unsigned int x, int c) { return x << c; }
int binop_lsh_hi(unsigned short x, int c) { return x << c; }
int binop_lsh_qi(unsigned char x, int c) { return x << c; }

#include "binops.inc"

#define DEF_BINOP(output, left, right, op, suffix)            \
    output binop_##suffix(left l, right r) { return l op r; }

#define BINOP_ACTION DEF_BINOP
BINOP_LIST

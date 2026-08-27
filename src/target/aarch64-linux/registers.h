#ifndef TACC_AARCH64_LINUX_REGISTERS_H
#define TACC_AARCH64_LINUX_REGISTERS_H

enum tacc_target_register {
    REG_X0 = 0x1,
    REG_X1 = 0x2,
    REG_X2 = 0x4,
    REG_X3 = 0x8,
    REG_X4 = 0x10,
    REG_X5 = 0x20,
    REG_X6 = 0x40,
    REG_X7 = 0x80,
    REG_X8 = 0x100,
    REG_X9 = 0x200,
    REG_X10 = 0x400,
    REG_X11 = 0x800,
    REG_X12 = 0x1000,
    REG_X13 = 0x2000,
    REG_X14 = 0x4000,
    REG_X15 = 0x8000,
    REG_X19 = 0x10000,
    REG_X20 = 0x20000,
    REG_X21 = 0x40000,
    REG_X22 = 0x80000,
    REG_X23 = 0x100000,
    REG_X24 = 0x200000,
    REG_X25 = 0x400000,
    REG_X26 = 0x800000,
    REG_X27 = 0x1000000,
    REG_X28 = 0x2000000,
};

enum tacc_target_register_float {
    REGF_V0 = 0x1,
    REGF_V1 = 0x2,
    REGF_V2 = 0x4,
    REGF_V3 = 0x8,
    REGF_V4 = 0x10,
    REGF_V5 = 0x20,
    REGF_V6 = 0x40,
    REGF_V7 = 0x80,
    REGF_V8 = 0x100,
    REGF_V9 = 0x200,
    REGF_V10 = 0x400,
    REGF_V11 = 0x800,
    REGF_V12 = 0x1000,
    REGF_V13 = 0x2000,
    REGF_V14 = 0x4000,
    REGF_V15 = 0x8000,
    REGF_V16 = 0x10000,
    REGF_V17 = 0x20000,
    REGF_V18 = 0x40000,
    REGF_V19 = 0x80000,
    REGF_V20 = 0x100000,
    REGF_V21 = 0x200000,
    REGF_V22 = 0x400000,
    REGF_V23 = 0x800000,
    REGF_V24 = 0x1000000,
    REGF_V25 = 0x2000000,
    REGF_V26 = 0x4000000,
    REGF_V27 = 0x8000000,
    REGF_V28 = 0x10000000,
    REGF_V29 = 0x20000000,
    REGF_V30 = 0x40000000,
};

enum tacc_target_reg_class {
    REGC_INT_W,
    REGC_INT_X,
    REGC_FLOAT_S,
    REGC_FLOAT_D,
    REGC_FLOAT_Q,
};

#define REG_ANY 0x3FFFFFF
#define REG_NONVOLATILE 0x3FF0000
#define REG_VOLATILE 0xFFFF

#endif

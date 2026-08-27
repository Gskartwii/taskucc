#ifndef TACC_TARGET_RISCV64_LINUX_REGISTERS_H
#define TACC_TARGET_RISCV64_LINUX_REGISTERS_H

enum tacc_target_register {
    REG_T0 = 0x1,
    REG_T1 = 0x2,
    REG_S1 = 0x4,
    REG_A0 = 0x8,
    REG_A1 = 0x10,
    REG_A2 = 0x20,
    REG_A3 = 0x40,
    REG_A4 = 0x80,
    REG_A5 = 0x100,
    REG_A6 = 0x200,
    REG_A7 = 0x400,
    REG_S2 = 0x800,
    REG_S3 = 0x1000,
    REG_S4 = 0x2000,
    REG_S5 = 0x4000,
    REG_S6 = 0x8000,
    REG_S7 = 0x10000,
    REG_S8 = 0x20000,
    REG_S9 = 0x40000,
    REG_S10 = 0x80000,
    REG_S11 = 0x100000,
    REG_T3 = 0x200000,
    REG_T4 = 0x400000,
    REG_T5 = 0x800000,
    REG_T6 = 0x1000000,
};

enum tacc_target_register_float {
    REGF_T0 = 0x1,
    REGF_T1 = 0x2,
    REGF_T2 = 0x4,
    REGF_T3 = 0x8,
    REGF_T4 = 0x10,
    REGF_T5 = 0x20,
    REGF_T6 = 0x40,
    REGF_T7 = 0x80,
    /* no s0 used to preserve enum encoding space*/
    REGF_S1 = 0x100,
    REGF_A0 = 0x200,
    REGF_A1 = 0x400,
    REGF_A2 = 0x800,
    REGF_A3 = 0x1000,
    REGF_A4 = 0x2000,
    REGF_A5 = 0x4000,
    REGF_A6 = 0x8000,
    REGF_A7 = 0x10000,
    REGF_S2 = 0x20000,
    REGF_S3 = 0x40000,
    REGF_S4 = 0x80000,
    REGF_S5 = 0x100000,
    REGF_S6 = 0x200000,
    REGF_S7 = 0x400000,
    REGF_S8 = 0x800000,
    REGF_S9 = 0x1000000,
    REGF_S10 = 0x2000000,
    REGF_S11 = 0x4000000,
    REGF_T8 = 0x8000000,
    REGF_T9 = 0x10000000,
    REGF_T10 = 0x20000000,
    REGF_T11 = 0x40000000,
};

enum tacc_target_reg_class {
    REGC_INT,
    REGC_FLOAT_S,
    REGC_FLOAT_D,
    /* We target rv64gc; do not assume existence of REGC_FLOAT_Q */
};

#define REG_ANY 0x1FFFFFF
#define REG_NONVOLATILE 0x1FF804
#define REG_VOLATILE 0x1E007FB

#define REGF_ANY 0x7FFFFFFF
#define REGF_NONVOLATILE 0x07FE0100
#define REGF_VOLATILE 0x7801FEFF

#endif

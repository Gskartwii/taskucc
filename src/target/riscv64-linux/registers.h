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

#define REG_ANY 0x1FFFFFF

#endif

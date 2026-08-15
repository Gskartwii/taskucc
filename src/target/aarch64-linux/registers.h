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

#define REG_ANY 0x3FFFFFF

#endif

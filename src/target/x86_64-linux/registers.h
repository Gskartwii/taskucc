#ifndef TACC_TARGET_X86_64_LINUX_REGISTERS_H
#define TACC_TARGET_X86_64_LINUX_REGISTERS_H

enum tacc_target_register {
    REG_RAX = 0x1,
    REG_RBX = 0x2,
    REG_RCX = 0x4,
    REG_RDX = 0x8,
    REG_RSI = 0x10,
    REG_RDI = 0x20,
    REG_R8 = 0x40,
    REG_R9 = 0x80,
    REG_R10 = 0x100,
    REG_R11 = 0x200,
    REG_R12 = 0x400,
    REG_R13 = 0x800,
    REG_R14 = 0x1000,
    REG_R15 = 0x2000,
};

#define REG_ANY 0x3FFF

#endif

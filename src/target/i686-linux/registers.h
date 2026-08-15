#ifndef TACC_TARGET_I686_LINUX_REGISTERS_H
#define TACC_TARGET_I686_LINUX_REGISTERS_H

enum tacc_target_register {
    REG_EAX = 0x1,
    REG_EBX = 0x2,
    REG_ECX = 0x4,
    REG_EDX = 0x8,
    REG_ESI = 0x10,
    REG_EDI = 0x20,
};

#define REG_ANY 0x3F

#endif

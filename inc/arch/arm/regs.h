#ifndef ARM_REGS_H
#define ARM_REGS_H

#include <types.h>

typedef struct __attribute__((packed)) saved_regs_arm_struct_t {
    /* Argument and result registers are X0->X7 */
    u64 x0;
    u64 x1;
    u64 x2;
    u64 x3;
    u64 x4;
    u64 x5;
    u64 x6;
    u64 x7;

    /* Caller saved X8->X15 */
    u64 x8;
    u64 x9;
    u64 x10;
    u64 x11;
    u64 x12;
    u64 x13;
    u64 x14;
    u64 x15;

    /* Intra-procedural call scratch registers */
    u64 x16;
    u64 x17;

    /* Platform register */
    u64 x18;

    /* Callee saved X19-X29 */
    u64 x19;
    u64 x20;
    u64 x21;
    u64 x22;
    u64 x23;
    u64 x24;
    u64 x25;
    u64 x26;
    u64 x27;
    u64 x28;

    /* Frame pointer */
    u64 x29;

    /* Link register */
    u64 x30;

    u64 sp_el0;
    u64 sp_el1;
    u64 spsr;
    u64 elr;
    u64 esr;
    u64 far;
} regs_t;

typedef struct {
    u8 vecregs[512];
    u64 fpsr;
    u64 fpcr;
} floatvec_regs_t;

#endif // ARM_REGS_H

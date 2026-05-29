#ifndef RV_REGS_H
#define RV_REGS_H

#include <types.h>

#define ARCH_HAS_GLOBAL_POINTER

typedef struct __attribute__((packed)) saved_regs_rv_struct_t {
    /* Return Address */
    u64 x1;

    /* Stack Pointer */
    u64 x2;

    u64 x3;
    u64 x4;
    u64 x5;
    u64 x6;
    u64 x7;
    u64 x8;
    u64 x9;

    /* Args 0->7 */
    u64 x10; // x10 and x11 are return value as well
    u64 x11;
    u64 x12;
    u64 x13;
    u64 x14;
    u64 x15;
    u64 x16;
    u64 x17;

    u64 x18;
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
    u64 x29;
    u64 x30;
    u64 x31;

    u64 sepc;
    u64 sstatus;
    u64 sscratch;
    u64 scause;
    u64 stval;
    u64 sie;
} regs_t;

typedef struct {
    u64 fpreg[32];
    u64 fcsr;
} floatvec_regs_t;

#endif // RV_REGS_H

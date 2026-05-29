#ifndef X86_REGS_H
#define X86_REGS_H

#include <types.h>

typedef struct __attribute__((packed)) saved_regs_x86_struct_t {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rdi;
    u64 rsi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 cr2;
#ifdef USE_TLS
    u64 fs; /* Used for TLS */
#endif // USE_TLS

    u64 interrupt_number;
    u64 error_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} regs_t;

typedef struct {
    u8 fxstor_region[512];
} floatvec_regs_t;

#endif // X86_REGS_H

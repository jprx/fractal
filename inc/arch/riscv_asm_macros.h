#ifndef RISCV_ASM_MACROS_H
#define RISCV_ASM_MACROS_H

#include <arch/riscv_asm_defines.h>

.macro PUSH r
    addi sp, sp, -8
    sd \r, 0(sp)
.endm

.macro POP r
    ld \r, 0(sp)
    addi sp, sp, 8
.endm

.macro PUSH_ALL
    addi sp, sp, -(RV_REGS_SIZE)
    sd x1, RV_REGS_X1(sp)
    # Skipping X2 (stack pointer / sp)
    sd x3, RV_REGS_X3(sp)
    sd x4, RV_REGS_X4(sp)
    sd x5, RV_REGS_X5(sp)
    sd x6, RV_REGS_X6(sp)
    sd x7, RV_REGS_X7(sp)
    sd x8, RV_REGS_X8(sp)
    sd x9, RV_REGS_X9(sp)
    sd x10, RV_REGS_X10(sp)
    sd x11, RV_REGS_X11(sp)
    sd x12, RV_REGS_X12(sp)
    sd x13, RV_REGS_X13(sp)
    sd x14, RV_REGS_X14(sp)
    sd x15, RV_REGS_X15(sp)
    sd x16, RV_REGS_X16(sp)
    sd x17, RV_REGS_X17(sp)
    sd x18, RV_REGS_X18(sp)
    sd x19, RV_REGS_X19(sp)
    sd x20, RV_REGS_X20(sp)
    sd x21, RV_REGS_X21(sp)
    sd x22, RV_REGS_X22(sp)
    sd x23, RV_REGS_X23(sp)
    sd x24, RV_REGS_X24(sp)
    sd x25, RV_REGS_X25(sp)
    sd x26, RV_REGS_X26(sp)
    sd x27, RV_REGS_X27(sp)
    sd x28, RV_REGS_X28(sp)
    sd x29, RV_REGS_X29(sp)
    sd x30, RV_REGS_X30(sp)
    sd x31, RV_REGS_X31(sp)
    addi x5, sp, (RV_REGS_SIZE)
    sd x5, RV_REGS_X2(sp)
    csrr x5, sepc
    sd x5, RV_REGS_SEPC(sp)
    csrr x5, sstatus
    sd x5, RV_REGS_SSTATUS(sp)
    csrr x5, sscratch
    sd x5, RV_REGS_SSCRATCH(sp)
    csrr x5, sie
    sd x5, RV_REGS_SIE(sp)
    csrr x5, scause
    sd x5, RV_REGS_SCAUSE(sp)
    csrr x5, stval
    sd x5, RV_REGS_STVAL(sp)
    ld x5, RV_REGS_X5(sp)
.endm

.macro POP_ALL
    ld x5, RV_REGS_STVAL(sp)
    csrw stval, x5
    ld x5, RV_REGS_SCAUSE(sp)
    csrw scause, x5
    ld x5, RV_REGS_SIE(sp)
    csrw sie, x5
    ld x5, RV_REGS_SSCRATCH(sp)
    csrw sscratch, x5
    ld x5, RV_REGS_SSTATUS(sp)
    csrw sstatus, x5
    ld x5, RV_REGS_SEPC(sp)
    csrw sepc, x5
    ld x1, RV_REGS_X1(sp)
    ld x3, RV_REGS_X3(sp)
    ld x4, RV_REGS_X4(sp)
    ld x5, RV_REGS_X5(sp)
    ld x6, RV_REGS_X6(sp)
    ld x7, RV_REGS_X7(sp)
    ld x8, RV_REGS_X8(sp)
    ld x9, RV_REGS_X9(sp)
    ld x10, RV_REGS_X10(sp)
    ld x11, RV_REGS_X11(sp)
    ld x12, RV_REGS_X12(sp)
    ld x13, RV_REGS_X13(sp)
    ld x14, RV_REGS_X14(sp)
    ld x15, RV_REGS_X15(sp)
    ld x16, RV_REGS_X16(sp)
    ld x17, RV_REGS_X17(sp)
    ld x18, RV_REGS_X18(sp)
    ld x19, RV_REGS_X19(sp)
    ld x20, RV_REGS_X20(sp)
    ld x21, RV_REGS_X21(sp)
    ld x22, RV_REGS_X22(sp)
    ld x23, RV_REGS_X23(sp)
    ld x24, RV_REGS_X24(sp)
    ld x25, RV_REGS_X25(sp)
    ld x26, RV_REGS_X26(sp)
    ld x27, RV_REGS_X27(sp)
    ld x28, RV_REGS_X28(sp)
    ld x29, RV_REGS_X29(sp)
    ld x30, RV_REGS_X30(sp)
    ld x31, RV_REGS_X31(sp)

    ld x2, RV_REGS_X2(sp)
.endm

// ALIAS_STACK_IF_KERNEL_TASK
// If this is a kernel task, change sp to its kernel alias.
// Clobbers a few regs, should be called with caller saved regs saved.
.macro ALIAS_STACK_IF_KERNEL_TASK
    // Is the stack already in kernel memory?
    // (either user task, or Oxide, which
    // uses a special stack)
    li a0, KERNEL_LINK_ADDRESS
    bgtu sp, a0, 1f

    // Are we a kernel task?
    la a0, cur_thread
    ld a0, 0(a0)
    beq zero, a0, 1f
    ld a1, THREAD_KIND(a0)
    li a2, THREAD_KIND_KERNEL_OUTER_THREAD
    bne a1, a2, 1f

    // We are a kernel task, re-align stack
    // (Assumption: large page used for stack)
    li a2, (RISCV_LARGE_PAGE_SIZE - 1)
    and a2, sp, a2

    ld a1, THREAD_RUN_STACK_PAGE(a0)
    add sp, a1, a2

1:  nop
.endm

#endif // RISCV_ASM_MACROS_H

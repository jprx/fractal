#ifndef ARM_ASM_MACROS_H
#define ARM_ASM_MACROS_H

#include <arch/arm_asm_defines.h>

.macro MOV64 r, imm
    movz \r, #(((\imm) >> 48) & 0x0FFFF), lsl #48
    movk \r, #(((\imm) >> 32) & 0x0FFFF), lsl #32
    movk \r, #(((\imm) >> 16) & 0x0FFFF), lsl #16
    movk \r, #(((\imm) >> 00) & 0x0FFFF), lsl #00
.endm

.macro LEA64 r, symbol
    adrp \r, \symbol
    add \r, \r, :lo12:\symbol
.endm

.macro PUSH r
    add sp, sp, -8
    str \r, [sp]
.endm

.macro POP r
    ldr \r, [sp]
    add sp, sp, 8
.endm

.macro PUSH_ALL
    add sp, sp, -(ARM_REGS_SIZE)
    str x0,  [sp,ARM_REGS_X0]
    str x1,  [sp,ARM_REGS_X1]
    str x2,  [sp,ARM_REGS_X2]
    str x3,  [sp,ARM_REGS_X3]
    str x4,  [sp,ARM_REGS_X4]
    str x5,  [sp,ARM_REGS_X5]
    str x6,  [sp,ARM_REGS_X6]
    str x7,  [sp,ARM_REGS_X7]
    str x8,  [sp,ARM_REGS_X8]
    str x9,  [sp,ARM_REGS_X9]
    str x10, [sp,ARM_REGS_X10]
    str x11, [sp,ARM_REGS_X11]
    str x12, [sp,ARM_REGS_X12]
    str x13, [sp,ARM_REGS_X13]
    str x14, [sp,ARM_REGS_X14]
    str x15, [sp,ARM_REGS_X15]
    str x16, [sp,ARM_REGS_X16]
    str x17, [sp,ARM_REGS_X17]
    str x18, [sp,ARM_REGS_X18]
    str x19, [sp,ARM_REGS_X19]
    str x20, [sp,ARM_REGS_X20]
    str x21, [sp,ARM_REGS_X21]
    str x22, [sp,ARM_REGS_X22]
    str x23, [sp,ARM_REGS_X23]
    str x24, [sp,ARM_REGS_X24]
    str x25, [sp,ARM_REGS_X25]
    str x26, [sp,ARM_REGS_X26]
    str x27, [sp,ARM_REGS_X27]
    str x28, [sp,ARM_REGS_X28]
    str x29, [sp,ARM_REGS_X29]
    str x30, [sp,ARM_REGS_X30]
    add x0, sp, ARM_REGS_SIZE
    str x0,  [sp,ARM_REGS_SP_EL1]
    mrs x0, spsr_el1
    str x0,  [sp,ARM_REGS_SPSR]
    mrs x0, sp_el0
    str x0,  [sp,ARM_REGS_SP_EL0]
    mrs x0, elr_el1
    str x0,  [sp,ARM_REGS_ELR]
    mrs x0, esr_el1
    str x0,  [sp,ARM_REGS_ESR]
    mrs x0, far_el1
    str x0,  [sp,ARM_REGS_FAR]
    ldr x0,  [sp,ARM_REGS_X0]
.endm

.macro POP_ALL
    mov x0, sp

    ldr x1,  [x0,ARM_REGS_SP_EL1]
    mov sp, x1
    ldr x1,  [x0,ARM_REGS_FAR]
    msr far_el1, x1
    ldr x1,  [x0,ARM_REGS_ESR]
    msr esr_el1, x1
    ldr x1,  [x0,ARM_REGS_ELR]
    msr elr_el1, x1
    ldr x1,  [x0,ARM_REGS_SP_EL0]
    msr sp_el0, x1
    ldr x1,  [x0,ARM_REGS_SPSR]
    msr spsr_el1, x1
    ldr x1,  [x0,ARM_REGS_X1]
    ldr x2,  [x0,ARM_REGS_X2]
    ldr x3,  [x0,ARM_REGS_X3]
    ldr x4,  [x0,ARM_REGS_X4]
    ldr x5,  [x0,ARM_REGS_X5]
    ldr x6,  [x0,ARM_REGS_X6]
    ldr x7,  [x0,ARM_REGS_X7]
    ldr x8,  [x0,ARM_REGS_X8]
    ldr x9,  [x0,ARM_REGS_X9]
    ldr x10, [x0,ARM_REGS_X10]
    ldr x11, [x0,ARM_REGS_X11]
    ldr x12, [x0,ARM_REGS_X12]
    ldr x13, [x0,ARM_REGS_X13]
    ldr x14, [x0,ARM_REGS_X14]
    ldr x15, [x0,ARM_REGS_X15]
    ldr x16, [x0,ARM_REGS_X16]
    ldr x17, [x0,ARM_REGS_X17]
    ldr x18, [x0,ARM_REGS_X18]
    ldr x19, [x0,ARM_REGS_X19]
    ldr x20, [x0,ARM_REGS_X20]
    ldr x21, [x0,ARM_REGS_X21]
    ldr x22, [x0,ARM_REGS_X22]
    ldr x23, [x0,ARM_REGS_X23]
    ldr x24, [x0,ARM_REGS_X24]
    ldr x25, [x0,ARM_REGS_X25]
    ldr x26, [x0,ARM_REGS_X26]
    ldr x27, [x0,ARM_REGS_X27]
    ldr x28, [x0,ARM_REGS_X28]
    ldr x29, [x0,ARM_REGS_X29]
    ldr x30, [x0,ARM_REGS_X30]

    ldr x0,  [x0,ARM_REGS_X0]
.endm

// ALIAS_STACK_IF_KERNEL_TASK
// If this is a kernel task, change sp to its kernel alias.
// Clobbers a few regs, should be called with caller saved regs saved.
.macro ALIAS_STACK_IF_KERNEL_TASK
    // Is the stack already in kernel memory?
    // (either user task, or Oxide, which
    // uses a special stack)
    MOV64 x0, KERNEL_LINK_ADDRESS
    mov x1, sp
    cmp x0, x1
    bgt 1f

    // Are we a kernel task?
    LEA64 x0, cur_thread
    ldr x0, [x0]
    cmp x0, 0
    beq 1f
    ldr x2, [x0,THREAD_KIND]
    cmp x2, THREAD_KIND_KERNEL_OUTER_THREAD
    bne 1f

    // We are a kernel task, re-align stack
    // (Assumption: large page used for stack)
    mov x1, sp
    MOV64 x2, (ARM_LARGE_PAGE_SIZE - 1)
    and x2, x1, x2

    ldr x1, [x0,THREAD_RUN_STACK_PAGE]
    add x2, x1, x2
    mov sp, x2

1:  nop
.endm

#endif // ARM_ASM_MACROS_H

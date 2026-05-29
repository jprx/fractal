.section .text
.global exception64_table
.global test_interrupts

#include <arch/arm_asm_macros.h>

.align 16
exception64_table:
// Current EL with SP0
    b sync_exception

.align 7
    b irq_exception

.align 7
    b fiq_exception

.align 7
    b serror_exception

// Current EL with SPx
.align 7
    b sync_exception

.align 7
    b irq_exception

.align 7
    b fiq_exception

.align 7
    b serror_exception

// Lower EL with AARCH64
.align 7
    b sync_exception

.align 7
    b irq_exception

.align 7
    b fiq_exception

.align 7
    b serror_exception

// Lower EL with AARCH32 (unused)
.rept 256
    b unk_exception
.endr

sync_exception:
    PUSH_ALL
    ALIAS_STACK_IF_KERNEL_TASK
    mov x0, sp
    bl handle_arm_sync_exception
    POP_ALL
    eret

irq_exception:
    PUSH_ALL
    ALIAS_STACK_IF_KERNEL_TASK
    mov x0, sp
    bl handle_arm_irq
    POP_ALL
    eret

fiq_exception:
    PUSH_ALL
    ALIAS_STACK_IF_KERNEL_TASK
    mov x0, sp
    bl handle_arm_fiq
    POP_ALL
    eret

serror_exception:
    PUSH_ALL
    ALIAS_STACK_IF_KERNEL_TASK
    mov x0, sp
    bl handle_arm_serror
    POP_ALL
    eret

unk_exception:
    PUSH_ALL
    ALIAS_STACK_IF_KERNEL_TASK
    mov x0, sp
    bl handle_arm_unk_exception
    POP_ALL
    eret

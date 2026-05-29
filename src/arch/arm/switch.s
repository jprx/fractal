.section .text

#include <arch/arm_asm_macros.h>
#include <arch/arm_asm_defines.h>

.global switchout

/*
 * switchout
 * x0: Our task structure
 * x1: The new task structure
 */
switchout:
    cmp x0, 0
    beq switchout_restore_next

    PUSH_ALL
    mov x2, sp
    str x2, [x0,THREAD_SAVED_SP]

switchout_restore_next:
    cmp x1, 0
    beq switchout_panic

    ldr x2, [x1,THREAD_LAUNCHED]
    cmp w2, 0
    beq switchout_new_task

switchout_old_task_leave:
    /* Only reload saved SP if returning to a previous task */
    ldr x2, [x1,THREAD_SAVED_SP]
    mov sp, x2
    POP_ALL
    ret

switchout_new_task:
    mov x2, 1
    str x2, [x1,THREAD_LAUNCHED]

    /* Push and pop init registers using the old task's stack */
    add sp, sp, -ARM_REGS_SIZE
    add x1, x1, THREAD_INIT_REGS
    mov x0, sp
    mov x2, ARM_REGS_SIZE
    bl memcpy

    ldr x0, [sp, ARM_REGS_SP_EL0]
    msr sp_el0, x0
    POP_ALL
    eret

switchout_panic:
    adr x0, switchout_panic_str
    bl panic
    b .

switchout_panic_str:
    .string "Tried to switch to a task that doesn't exist!"

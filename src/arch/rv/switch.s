.section .text

#include <arch/riscv_asm_macros.h>
#include <arch/riscv_asm_defines.h>

.global switchout

/*
 * switchout
 * a0: Our task
 * a1: Next task
 */
switchout:
    beq a0, zero, switchout_restore_next
    PUSH_ALL
    sd sp, THREAD_SAVED_SP(a0)

switchout_restore_next:
    beq a1, zero, switchout_panic

    ld t0, THREAD_LAUNCHED(a1)
    beq t0, zero, switchout_new_task

switchout_old_task_leave:
    ld sp, THREAD_SAVED_SP(a1)
    POP_ALL
    ret

switchout_new_task:
    li t0, 1
    sd t0, THREAD_LAUNCHED(a1)

    add sp, sp, -RV_REGS_SIZE
    add a1, a1, THREAD_INIT_REGS
    mv a0, sp
    li a2, RV_REGS_SIZE
    call memcpy

    POP_ALL
    sret

switchout_panic:
    la a0, switchout_panic_str
    jal panic
    j .

switchout_panic_str:
    .string "Tried to switch to a task that doesn't exist!"

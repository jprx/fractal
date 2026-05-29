.intel_syntax noprefix
.section .text
.code64

#include <arch/x86_asm_macros.h>
#include <arch/x86_asm_defines.h>

.global switchout


/*
 * switchout
 * rdi: Our task structure
 * rsi: The new task structure
 */
switchout:
    // Are we switching from a real task? If not, skip the save step
    cmp rdi, 0
    je switchout_restore_next

    // Save our registers to the stack
    // in general, this just needs to be callee saved, but let's do them all
    PUSH_ALL_NO_IRETCTX
    // Regs are free to be clobbered now

    mov [rdi+THREAD_SAVED_SP], rsp

switchout_restore_next:
    cmp rsi, 0
    je switchout_panic

    mov rdi, [rsi+THREAD_LAUNCHED]
    cmp rdi, 0
    je switchout_new_task

switchout_old_task_leave:
    mov rsp, [rsi+THREAD_SAVED_SP]
    POP_ALL_NO_IRETCTX
    ret

switchout_new_task:
    mov rdi, 1
    mov [rsi+THREAD_LAUNCHED], rdi

    // Copy new_task->t_init_regs to stack
    // memcpy(new_kernel_stack, task.t_init_regs)
    // This copies everything, including the fake iret context :)
    add rsp, -X86_REGS_SIZE
    lea rsi, [rsi+THREAD_INIT_REGS]
    mov rdi, rsp
    mov rdx, X86_REGS_SIZE
    call memcpy

    // Now this is the same as the teardown of a DECLARE_INTERRUPT C call
    POP_ALL_NO_IRETCTX

    // Throw away fake error code and interrupt number:
    add sp, X86_IRET_CONTEXT_JUST_ERROR_AND_INTNUM_SIZE

    // And that's all
    iretq

switchout_panic:
    lea rdi, [rip + switchout_panic_str]
    call panic
    jmp .

switchout_panic_str:
    .string "Tried to switch to a task that doesn't exist!"

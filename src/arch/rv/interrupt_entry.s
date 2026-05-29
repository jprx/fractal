.section .text
.global user_trap, kernel_trap
.global test_interrupts

#include <arch/riscv_asm_macros.h>

.align 4
user_trap:
    // This is the first interrupt from a task
    // Swap to kernel stack and push regs there
    // All future interrupts will happen on the kernel stack, so switch stvec to that, too
    // Swap sp for sscratch, and vice versa
    csrrw sp, sscratch, sp

    // sscratch holds sp, and sp holds the kernel stack
    PUSH_ALL

    // Correct pushed regs by swapping saved sscratch with saved X2
    // When these are popped sscratch will be reloaded with the saved kernel stack pointer
    ld a0, RV_REGS_SSCRATCH(sp)
    ld a1, RV_REGS_X2(sp)
    sd a0, RV_REGS_X2(sp)
    sd a1, RV_REGS_SSCRATCH(sp)

    // All traps until this returns need to use kernel_trap
    la a0, kernel_trap
    csrw stvec, a0

    mv a0, sp
    call rv_handle_trap

    POP_ALL
    sret

kernel_trap:
    // A trap from kernelspace- could be the first trap from a kernel ELF task,
    // or an interrupt / exception encountered during kernel code exec.
    // Might need to fixup the kernel stack, if we are a kernel ELF task.
    PUSH_ALL
    ALIAS_STACK_IF_KERNEL_TASK
    mv a0, sp
    call rv_handle_trap
    POP_ALL
    sret

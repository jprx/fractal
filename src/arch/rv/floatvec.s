.section .text
.global thread_save_floatvec_regs
.global thread_load_floatvec_regs

#include <arch/riscv_asm_macros.h>
#include <arch/riscv_asm_defines.h>

.altmacro

// save_fpreg(t3=ptr to task->saved_floatvecs)
.macro save_fpreg i
    fsd f\i, ((\i) * RISCV_FPREG_SZ)(t3)
.endm

// save_all_fpregs(t3=ptr to task->saved_floatvecs)
.macro save_all_fpregs
.set reg_idx, 0
.rept RISCV_NUM_FPREGS
    save_fpreg %(reg_idx)
    .set reg_idx, reg_idx+1
.endr
csrr t0, fcsr
sd t0, FLOATVEC_FCSR(t3)
.endm

// load_fpreg(t3=ptr to task->saved_floatvecs)
.macro load_fpreg i
    fld f\i, ((\i) * RISCV_FPREG_SZ)(t3)
.endm

// load_all_fpregs(t3=ptr to task->saved_floatvecs)
.macro load_all_fpregs
.set reg_idx, 0
.rept RISCV_NUM_FPREGS
    load_fpreg %(reg_idx)
    .set reg_idx, reg_idx+1
.endr
ld t0, FLOATVEC_FCSR(t3)
csrw fcsr, t0
.endm

// void thread_save_floatvec_regs(thread_t *t);
thread_save_floatvec_regs:
    addi t3, a0, THREAD_SAVED_FLOATVEC
    save_all_fpregs
    ret

// void thread_load_floatvec_regs(thread_t *t);
thread_load_floatvec_regs:
    addi t3, a0, THREAD_SAVED_FLOATVEC
    load_all_fpregs
    ret

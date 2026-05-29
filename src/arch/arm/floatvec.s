.section .text
.global thread_save_floatvec_regs
.global thread_load_floatvec_regs

#include <arch/arm_asm_macros.h>
#include <arch/arm_asm_defines.h>

.altmacro

// save_qreg(x0=ptr to task->saved_floatvecs)
.macro save_qreg i i2
    stp q\i, q\i2, [x0, #(\i * ARM_VECREG_SZ)]
.endm

// save_all_qregs(x0=ptr to task->saved_floatvecs)
.macro save_all_qregs
.set reg_idx, 0
.rept ARM_NUM_VECREGS / 2
    save_qreg %(reg_idx), %(reg_idx+1)
    .set reg_idx, reg_idx+2
.endr
mrs x9, fpsr
str x9, [x0, FLOATVEC_FPSR]
mrs x9, fpcr
str x9, [x0, FLOATVEC_FPCR]
.endm

// load_qreg(x0=ptr to task->saved_floatvecs)
.macro load_qreg i i2
    ldp q\i, q\i2, [x0, #(\i * ARM_VECREG_SZ)]
.endm

// load_all_qregs(x0=ptr to task->saved_floatvecs)
.macro load_all_qregs
.set reg_idx, 0
.rept ARM_NUM_VECREGS / 2
    load_qreg %(reg_idx), %(reg_idx+1)
    .set reg_idx, reg_idx+2
.endr
ldr x9, [x0, FLOATVEC_FPSR]
msr fpsr, x9
ldr x9, [x0, FLOATVEC_FPCR]
msr fpcr, x9
.endm

// void thread_save_floatvec_regs(thread_t *t);
// x0 and x9 are both caller-saved, but we save them anyways to be extra safe
thread_save_floatvec_regs:
    PUSH x0
    PUSH x9
    add x0, x0, THREAD_SAVED_FLOATVEC
    save_all_qregs
    POP x9
    POP x0
    ret

// void thread_load_floatvec_regs(thread_t *t);
// x0 and x9 are both caller-saved, but we save them anyways to be extra safe
thread_load_floatvec_regs:
    PUSH x0
    PUSH x9
    add x0, x0, THREAD_SAVED_FLOATVEC
    load_all_qregs
    POP x9
    POP x0
    ret

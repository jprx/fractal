.intel_syntax noprefix
.code64
.section .text
.global thread_save_floatvec_regs
.global thread_load_floatvec_regs

#include <arch/x86_asm_macros.h>
#include <arch/x86_asm_defines.h>

// void thread_save_floatvec_regs(thread_t *t);
thread_save_floatvec_regs:
    fxsave [rdi+THREAD_SAVED_FLOATVEC]
    ret

// void thread_load_floatvec_regs(thread_t *t);
thread_load_floatvec_regs:
    fxrstor [rdi+THREAD_SAVED_FLOATVEC]
    ret

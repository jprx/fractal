.section .text.start64
.global fractal_start64
.extern exception64_table

#include <arch/arm_asm_macros.h>

/* Landing pad for higher half code */
fractal_start64:
    adr x0, exception64_table
    msr vbar_el1, x0
    isb

    /* This overwrites the stack we used during start.s,
     * but that's ok because we never go back / refer to it again */
    LEA64 x0, _stack_bottom_lma
    mov sp, x0
    mov x0, x27
    mov x1, x26
    b fractal_main

.global get_current_sp
get_current_sp:
    mov x0, sp
    ret

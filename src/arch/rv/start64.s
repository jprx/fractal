.section .text.start64
.global fractal_start64

/* Higher half landing pad */
fractal_start64:
    la sp, _stack_bottom_lma
    j fractal_main

.global get_current_sp
get_current_sp:
    mv a0, sp
    ret

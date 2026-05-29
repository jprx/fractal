.section .text

#include <arch/arm_asm_defines.h>

// Something to just run and never stop
.align 12
.global init_task
init_task:
	b .


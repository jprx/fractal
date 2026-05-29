.intel_syntax noprefix
.section .text

#include <arch/x86_asm_defines.h>

// Something to just run and never stop
.align 4096
.global init_task
init_task:
	jmp .


.intel_syntax noprefix
.code64
.section .text.start64
.global fractal_start64
#include <arch/x86_asm_defines.h>

/* This is called after setting up paging and long mode in start.s */
fractal_start64:
    // Try to enable FSGSBASE instructions
    // This doesn't work with -machine q35 sadly
    // With -cpu max it does work, but initrd moves around
    // and multiboot header is invalid for some reason
    //mov rax, cr4
    //or rax, CR4_FSGSBASE
    //mov cr4, rax

    lea rax, [rip + global_multiboot_ptr]
    mov [rax], rbx
    lea rax, [rip + _stack_bottom_lma]
    mov rsp, rax
    call fractal_main

.global get_current_sp
get_current_sp:
    mov rax, rsp
    ret

.global global_multiboot_ptr
.align 8
global_multiboot_ptr: .quad 0

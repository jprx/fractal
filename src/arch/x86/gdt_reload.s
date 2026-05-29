.intel_syntax noprefix
.section .text
.global reload_gdt
.code64

#include <arch/x86/selector.h>

reload_gdt:
    lgdt [rdi]
    mov ax, KERNEL_DS
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push KERNEL_CS
    lea rax, [rip + reload_gdt_done]
    push rax
    retfq

reload_gdt_done:
    ret

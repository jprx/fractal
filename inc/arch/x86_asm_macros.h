#ifndef X86_ASM_MACROS_H
#define X86_ASM_MACROS_H

.intel_syntax noprefix

#define FSBASE_MSR       0xC0000100
#define GSBASE_MSR       0xC0000101

#include <arch/x86_asm_defines.h>

// PUSH_ALL_NO_IRETCTX- push all registers to the stack
// Assumes an iret context WITH error code exists below,
// so we don't have to save rip, rsp, cs, ss, rflags
.macro PUSH_ALL_NO_IRETCTX
    add rsp, -X86_REGS_SIZE
    add rsp, X86_IRET_CONTEXT_SIZE_WITH_ERROR_AND_INTNUM
    mov [rsp+X86_REGS_RAX],  rax
    mov [rsp+X86_REGS_RBX],  rbx
    mov [rsp+X86_REGS_RCX],  rcx
    mov [rsp+X86_REGS_RDX],  rdx
    mov [rsp+X86_REGS_RDI],  rdi
    mov [rsp+X86_REGS_RSI],  rsi
    mov [rsp+X86_REGS_RBP],  rbp
    mov [rsp+X86_REGS_R8],   r8
    mov [rsp+X86_REGS_R9],   r9
    mov [rsp+X86_REGS_R10],  r10
    mov [rsp+X86_REGS_R11],  r11
    mov [rsp+X86_REGS_R12],  r12
    mov [rsp+X86_REGS_R13],  r13
    mov [rsp+X86_REGS_R14],  r14
    mov [rsp+X86_REGS_R15],  r15

    // Special regs- use rax as temp
    mov rax, cr2
    mov [rsp+X86_REGS_CR2],  rax

#ifdef USE_TLS
    mov rcx, FSBASE_MSR
    rdmsr
    shl rdx, 32
    or rax, rdx
    mov [rsp+X86_REGS_FS], rax
#endif // USE_TLS

    mov rax, [rsp+X86_REGS_RAX]
#ifdef USE_TLS
    mov rcx, [rsp+X86_REGS_RCX]
    mov rdx, [rsp+X86_REGS_RDX]
#endif // USE_TLS
.endm

// POP_ALL_NO_IRETCTX- pop all registers from stack
// Assumes an iret context WITH error code exists below,
// so we don't restore rip, rsp, cs, ss, rflags, or pop the error code off
.macro POP_ALL_NO_IRETCTX
    mov rax, [rsp+X86_REGS_CR2]
    mov cr2, rax
#ifdef USE_TLS
    // wrfsbase rax doesn't work bc cr4[FSGSBASE] not supported for q35 machine
    mov rax, [rsp+X86_REGS_FS]
    mov rcx, FSBASE_MSR
    mov rdx, rax
    shr rdx, 32
    wrmsr
#endif // USE_TLS
    mov rax, [rsp+X86_REGS_RAX]
    mov rbx, [rsp+X86_REGS_RBX]
    mov rcx, [rsp+X86_REGS_RCX]
    mov rdx, [rsp+X86_REGS_RDX]
    mov rdi, [rsp+X86_REGS_RDI]
    mov rsi, [rsp+X86_REGS_RSI]
    mov rbp, [rsp+X86_REGS_RBP]
    mov r8, [rsp+X86_REGS_R8]
    mov r9, [rsp+X86_REGS_R9]
    mov r10, [rsp+X86_REGS_R10]
    mov r11, [rsp+X86_REGS_R11]
    mov r12, [rsp+X86_REGS_R12]
    mov r13, [rsp+X86_REGS_R13]
    mov r14, [rsp+X86_REGS_R14]
    mov r15, [rsp+X86_REGS_R15]
    add rsp, X86_REGS_SIZE
    add rsp, -X86_IRET_CONTEXT_SIZE_WITH_ERROR_AND_INTNUM
.endm

// DECLARE_INTERRUPT and DECLARE_INTERRUPT_WITH_ERROR
// both setup an iret ctx (with fake error as needed) and
// proceed to feed into `x86_generic_int`.

// DECLARE_INTERRUPT
// Declares a new interrupt with number idx.
// name: The name of the assembly stub to export (eg. something to load into the IDT)
// idx: The number we push before calling x86_generic_int
.macro DECLARE_INTERRUPT idx,name
.global \name
\name:
    push 0
    push \idx
    jmp x86_generic_int
.endm

// DECLARE_INTERRUPT_WITH_ERROR
// We assume this interrupt (really, exception) pushed an error code already
.macro DECLARE_INTERRUPT_WITH_ERROR idx,name
.global \name
\name:
    push \idx
    jmp x86_generic_int
.endm

// ALIAS_STACK_IF_KERNEL_TASK
// If this is a kernel task, change rsp to its kernel alias.
// Clobbers a few regs, should be called with caller saved regs saved.
.macro ALIAS_STACK_IF_KERNEL_TASK
    // Is the stack already in kernel memory?
    // (either user task, or Oxide, which
    // uses a special stack)
    mov r10, KERNEL_LINK_ADDRESS
    cmp r10, rsp
    jb 1f

    // Are we a kernel task?
    lea r8, [rip + cur_thread]
    mov r8, [r8]
    cmp r8, 0
    je 1f
    mov r9, [r8 + THREAD_KIND]
    cmp r9, THREAD_KIND_KERNEL_OUTER_THREAD
    jne 1f

    // We are a kernel task, re-align stack
    // (Assumption: large page used for stack)
    mov r10, rsp
    mov r9, (X86_LARGE_PAGE_SIZE - 1)
    and r9, r10

    mov r10, [r8 + THREAD_RUN_STACK_PAGE]
    add r9, r10
    mov rsp, r9
1:  nop
.endm

#endif // X86_ASM_MACROS_H

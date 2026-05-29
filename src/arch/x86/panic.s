.intel_syntax noprefix
.section .text
.global panic
.global test_panic
#include <arch/x86_asm_defines.h>

# panic(const char *reason, ...)
# This calls panic(saved_regs_t *r, const char *reason, ...) by
# saving regs to stack and swapping args around
panic:
    # Immediately push flags so we don't clobber it
    # Stack is now:
    # +------------+
    # |   rflags   | <-- rsp
    # +------------+
    # |     rip    |
    # +------------+
    pushfq

    # Add 8 bytes to sizeof(regs_t) for padding to be safe
    add rsp, -X86_REGS_SIZE
    add rsp, -8
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

    # Error code (0)
    xor rdi, rdi
    mov [rsp+X86_REGS_ERR], rdi

    # rip is what rsp used to point to + 8 more just before panic() began
    lea rdi, [rsp+X86_REGS_SIZE]
    add rdi, 16
    mov rdi, [rdi]
    mov [rsp+X86_REGS_RIP], rdi

    # cs
    xor rdi, rdi
    mov rdi, cs
    mov [rsp+X86_REGS_CS], rdi

    # rflags was pushed to stack at start here
    lea rdi, [rsp+X86_REGS_SIZE]
    add rdi, 8
    mov rdi, [rdi]
    mov [rsp+X86_REGS_RFL], rdi

    # rsp = sizeof(regs_t) + 8 bytes padding + 8 bytes for rflags
    lea rdi, [rsp+X86_REGS_SIZE]
    add rdi, 16
    mov [rsp+X86_REGS_RSP], rdi

    # ss
    xor rdi, rdi
    mov rdi, ss
    mov [rsp+X86_REGS_SS], rdi

    # swap around args
    mov r9, r8
    mov r8, rcx
    mov rcx, rdx
    mov rdx, rsi
    # rdi has been clobbered, reload old rdi from stack and store in rsi
    mov rsi, [rsp+X86_REGS_RDI]

    # rdi now points to the saved_regs_t
    mov rdi, rsp

	jmp _panic_internal
	# Does not return
	jmp .


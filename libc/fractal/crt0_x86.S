#ifndef FRACTAL_X86
#error "Including X86 crt0.S without being the FRACTAL_X86 target"
#endif // !FRACTAL_X86

.intel_syntax noprefix
.code64
.section .text.startup
.global _start

_start:
    // Save argc, argv, and envp
    push rdi
    push rsi
    push rdx

    // Copy argv[0] into __progname pointer
    lea r8, [rip + __progname]
    mov r9, [rsi]
    mov [r8], r9

    // Copy envp into environ, if provided
    test rdx, rdx
    jz skip_envp_cpy
    lea r8, [rip + environ]
    mov [r8], rdx

    // Setup constructors
    call __libc_init_array

    // Reset clobbered regs
    skip_envp_cpy:
    xor r8, r8
    xor r9, r9
    pop rdx
    pop rsi
    pop rdi

    call main
    mov rdi, rax
    mov rax, 2
    syscall
    jmp .

# See System V ABI Section A.2
# https://refspecs.linuxfoundation.org/elf/x86_64-abi-0.99.pdf
.global do_syscall
do_syscall:
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r10, r8
    mov r8, r9
    syscall
    ret


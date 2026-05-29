#ifndef X86_SYSCALL_H
#define X86_SYSCALL_H

#ifndef ARCH_X86
#error "X86 target not selected, but including syscall.h for X86"
#endif // !ARCH_X86

#include <fractal.h>

BEGIN_C_HEADER

void syscall_entry();
void syscall_compat_entry();

END_C_HEADER

#endif // X86_SYSCALL_H

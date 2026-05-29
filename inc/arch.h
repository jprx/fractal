#ifndef ARCH_H
#define ARCH_H

#if ARCH_X86
#include <arch/x86.h>
#endif // ARCH_X86

#if ARCH_ARM
#include <arch/arm.h>
#endif // ARCH_ARM

#if ARCH_RISCV
#include <arch/riscv.h>
#endif // ARCH_RISCV

#include <board.h>

extern FRACTALCORE_CLASS global_cpu;

#endif // ARCH_H

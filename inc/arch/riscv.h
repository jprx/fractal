#ifndef RISCV_H
#define RISCV_H

#include <arch/riscv_asm_defines.h>
#include <io/serial/uart16550.h>
#include <arch/rv/cpu.h>

#define FRACTALCORE_CLASS FractalCoreRV
#define FRACTALPTE_CLASS PTERV

extern u8 _stack_bottom_lma;
#define KERNEL_STACK ((((usize)&_stack_bottom_lma)))

#define RISCV_INSTRUCTION_SIZE 4

#endif // RISCV_H

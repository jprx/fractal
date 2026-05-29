#ifndef ARM_H
#define ARM_H

#include <fractal.h>
#include <arch/arm_asm_defines.h>
#include <arch/arm/cpu.h>

#define FRACTALCORE_CLASS FractalCoreARM
#define FRACTALPTE_CLASS PTEARM

BEGIN_C_HEADER

void arm_generic_timer_schedule_interrupt();
void arm_generic_timer_enable_interrupts();

extern u8 _stack_bottom_lma;
#define KERNEL_STACK ((((usize)&_stack_bottom_lma)))

END_C_HEADER

#endif // ARM_H

#include <arch/arm_asm_defines.h>
#define CNTP_CTL_INTERRUPTS_ENABLED 0b001
#define CNTV_CTL_INTERRUPTS_ENABLED 0b001

void arm_generic_timer_schedule_interrupt_physical_timer() {
    asm volatile(
        "msr CNTP_TVAL_EL0, %0"
        :: "r"(TIMESLICE_QUANTUM)
    );
}

void arm_generic_timer_enable_interrupts_physical_timer() {
    asm volatile(
        "msr CNTP_CTL_EL0, %0"
        :: "r"(CNTP_CTL_INTERRUPTS_ENABLED)
    );
}

// Must use virtual timer for Qemu HVF to function properly,
// as Apple's HVF does not virtualize the physical timer.

void arm_generic_timer_schedule_interrupt() {
    asm volatile(
        "msr CNTV_TVAL_EL0, %0"
        :: "r"(TIMESLICE_QUANTUM)
    );
}

void arm_generic_timer_enable_interrupts() {
    asm volatile(
        "msr CNTV_CTL_EL0, %0"
        :: "r"(CNTV_CTL_INTERRUPTS_ENABLED)
    );
}

#include <fractal.h>
#include <io/interrupt/lapic.h>
#include <io/interrupt/apic.h>
#include <interrupt.h>
#include <arch.h>
#include <types.h>
#include <out.h>

void LAPIC::CheckConfiguration() {
    u32 apic_bar_val = read_msr(APIC_BAR_MSR);

    if (GET_APIC_BASEADDR_FROM_BAR(apic_bar_val) != KERN_DV2P(LAPIC_DEFAULT_ADDRESS)) {
        panic("Failed to enable LAPIC: not at default address (at 0x%X instead)", GET_APIC_BASEADDR_FROM_BAR(apic_bar_val));
    }

#ifndef QUIET_BOOT
    if (APIC_BAR_IS_BSC(apic_bar_val)) {
        printf("We are the bootstrap core\r\n");
    } else {
        printf("We are not the bootstrap core\r\n");
    }
#endif // !QUIET_BOOT

    if ((usize)this != LAPIC_DEFAULT_ADDRESS) {
        panic("LAPIC CheckConfiguration called on LAPIC that is not at the default address");
    }
}

void LAPIC::Enable() {
    CheckConfiguration();

    // Enable APIC in the APIC BAR (AMD Vol 2 16.3.1 Local APIC Enable)
    write_msr(APIC_BAR_MSR, read_msr(APIC_BAR_MSR) | (1 << APIC_BAR_ENABLE));

    // We need to turn off LINT0 because the legacy PIC sends interrupts to it
	// We want to just use the IOAPIC and PCI MSI-X for interrupts, and forget the old PIC
	// Intel SDM says these fields should begin masked though
    WriteReg(LAPIC_LINT0, ReadReg(LAPIC_LINT0) | (1 << APIC_LINT0_DISABLE_BIT));

    // Spurious Interrupt Vector also randomly has the bit that turns on LAPIC,
    // so set that bit, too
    WriteReg(LAPIC_SPURIOUS_INT_VECTOR, ReadReg(LAPIC_SPURIOUS_INT_VECTOR) | (1 << APIC_SPURIOUS_INT_VECTOR_ENABLE_BIT));

    // This should be zero at reset, but we set it anyways because why not
    WriteReg(LAPIC_TASK_PRIORITY, 0);
}

u32 LAPIC::ReadReg(u32 reg) {
    volatile u32 *config_space = (volatile u32 *)this;
    return config_space[reg];
}

void LAPIC::WriteReg(u32 reg, u32 val) {
    volatile u32 *config_space = (volatile u32 *)this;
    config_space[reg] = val;
}

void LAPIC::SendEOI() {
    WriteReg(LAPIC_EOI, 0);
}

void LAPIC::EnableTimer() {
    // 1. Setup divide config reg
    WriteReg(LAPIC_DIVIDE_CONFIG_REG, LAPIC_DIVIDE_CONFIG_1TO1);

    // 2. Timer local vector table
    WriteReg(LAPIC_TIMER_LVT, LAPIC_TIMER_LVT_PERIODIC | APIC_TIMER_IRQ);

#ifndef QUIET_BOOT
    // WriteReg(LAPIC_TIMER_LVT, ReadReg(LAPIC_TIMER_LVT) & ~((1ull << 16ull)));
    printf("Timer LVT is 0x%X\r\n", ReadReg(LAPIC_TIMER_LVT));
    printf("LINT0 LVT is 0x%X\r\n", ReadReg(LAPIC_LINT0));
    printf("LINT1 LVT is 0x%X\r\n", ReadReg(LAPIC_LINT1));
#endif // !QUIET_BOOT

    // 3. Initial count register starts the timer
    WriteReg(LAPIC_TIMER_INITIAL_COUNT, TIMESLICE_QUANTUM);
}

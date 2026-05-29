#include <types.h>
#include <interrupt.h>
#include <io/interrupt_source.h>
#include <io/interrupt/gic.h>

// Fully configures distributor to send us this IRQ
static inline void enable_irq(virt_t gicd_base, u32 irq) {
    // ISENABLER has 32 irq's per register
    ((volatile u32*)GICD_ISENABLER(gicd_base))[irq / GICD_ISENABLER_IRQS_PER_REG] = (1 << (irq % GICD_ISENABLER_IRQS_PER_REG));

    if (irq >= GICD_FIRST_SPI_IRQNUM) {
        // This OR's in our CPU number into the enabled CPUs, but doesn't clear others
        ((volatile u32*)GICD_ITARGETSR(gicd_base))[irq / GICD_ITARGETSR_IRQS_PER_REG] |= (GICD_ITARGET_CORE0 << 8 * (irq % GICD_ITARGETSR_IRQS_PER_REG));
    }
}

FractalGIC::FractalGIC(virt_t d_in, virt_t c_in) {
    gicd_base = d_in;
    gicc_base = c_in;
}

void FractalGIC::Init() {
    // Enable distributor
    *(volatile u32*)GICD_CTLR(gicd_base) = 1;

    // Enable CPU GIC
    *(volatile u32*)GICC_CTLR(gicc_base) = 1;
    *(volatile u32*)GICC_PMR(gicc_base) = GIC_PRIORITY_ALL; // All interrupts allowed (higher value = lower priority)

#ifdef BOARD_QEMU
    // Enable and forward all interrupts to core 0
    // @TODO: Refactor this into a board-specific interrupts module.
    for (usize i = 0; i < GIC_NUM_INTERRUPTS; i++) {
        enable_irq(gicd_base, i);
    }
#endif // BOARD_QEMU

#ifdef BOARD_RASPI
    // Only explicitly enable interrupts we know about,
    // as other peripherals on RPi can be noisy.
    // @TODO: Refactor this into a board-specific interrupts module.
    enable_irq(gicd_base, VIRTUAL_TIMER_IRQ);
    enable_irq(gicd_base, UART0_IRQ);
#endif // BOARD_RASPI
}

InterruptID FractalGIC::ClaimInterrupt() {
    return *(volatile u32 *)GICC_IAR(gicc_base);
}

void FractalGIC::SendEOI(InterruptID i) {
    *(volatile u32*)GICC_EOIR(gicc_base) = i;
}

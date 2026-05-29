#include <io/interrupt/plic.h>

#define BITS_PER_U32 32

FractalPLIC::FractalPLIC(virt_t baseaddr_in) {
    baseaddr = baseaddr_in;
}

void FractalPLIC::Init() {
    u64 my_id = 0;

    // Set priority for UART0 to nonzero to enable it
    ((volatile u32 *)PLIC_INT_PRIORITY(baseaddr))[UART0_IRQ] = 1;

    // Enable UART0
    ((volatile u32 *)(PLIC_ENABLE(baseaddr,PLIC_SCONTEXT(my_id))))[0] = (1ull << UART0_IRQ);

    // Enable everything
    for (usize i = 0; i < PLIC_NUM_INTERRUPTS; i++) {
        ((volatile u32 *)PLIC_INT_PRIORITY(baseaddr))[i] = 1;
    }

    for (usize i = 0; i < PLIC_NUM_INTERRUPTS / BITS_PER_U32; i++) {
        ((volatile u32 *)(PLIC_ENABLE(baseaddr,PLIC_SCONTEXT(my_id))))[i] = 0xFFFFFFFF;
    }

    // Set priority threshold to 0 to enable all interrupts
    ((volatile u32 *)(PLIC_PRIORITY(baseaddr,PLIC_SCONTEXT(my_id))))[PLIC_PRIORITY_THRESHOLD] = 0;
}

InterruptID FractalPLIC::ClaimInterrupt() {
    u64 my_id = 0;
    return ((volatile u32 *)(PLIC_PRIORITY(baseaddr,PLIC_SCONTEXT(my_id))))[PLIC_PRIORITY_CLAIM_COMPLETE];
}

void FractalPLIC::SendEOI(InterruptID irq) {
    u64 my_id = 0;
    ((volatile u32 *)(PLIC_PRIORITY(baseaddr,PLIC_SCONTEXT(my_id))))[PLIC_PRIORITY_CLAIM_COMPLETE] = irq;
}

#include <fractal.h>
#include <arch.h>
#include <cpu.h>
#include <interrupt.h>

void FractalAPIC::Init() {
    // Init is handled by the LAPIC/ IOAPIC classes,
    // this is just a wrapper around those two
    return;
}

InterruptID FractalAPIC::ClaimInterrupt() {
    // No need to do this for an APIC
    return 0;
}

void FractalAPIC::SendEOI(InterruptID unused) {
    // Just send EOI, no need to relay the vector
    lapic->SendEOI();
}

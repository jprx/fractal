#ifndef APIC_H
#define APIC_H

#include <interrupt.h>
#include <io/interrupt_source.h>
#include <io/interrupt/lapic.h>
#include <io/interrupt/ioapic.h>

class FractalAPIC : public InterruptSource {
public:
    LAPIC *lapic;
    IOAPIC *ioapic;

    /*
     * Construct a new APIC at the given core-local baseaddr,
     * with a LAPIC and IOAPIC in the virtual address space.
     */
    FractalAPIC(LAPIC *l, IOAPIC *i) : lapic(l), ioapic(i) {}

    /*
     * FractalAPIC gets a special constructor unique to it
     * because we don't know where the APIC / IOAPIC are until
     * we parse the ACPI tables.
     *
     * Global constructor can just set them to NULL, and we
     * will fill them in during CPU interrupt init.
     */
    FractalAPIC() : lapic(NULL), ioapic(NULL) {}
    virtual void Init() override;
    virtual InterruptID ClaimInterrupt() override;
    virtual void SendEOI(InterruptID) override;
};

#define APIC_BAR_MSR ((0x1B))

#define APIC_BAR_BSC 8
#define APIC_BAR_ENABLE 11

#define GET_APIC_BASEADDR_FROM_BAR(b) (((b & ~0x0FFF)))

#define APIC_BAR_IS_BSC(b) ((b & (1 << APIC_BAR_BSC)) != 0)

#endif // APIC_H

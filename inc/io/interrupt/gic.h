#ifndef GIC_H
#define GIC_H

#include <types.h>
#include <interrupt.h>
#include <io/interrupt_source.h>
#include <virt_mem.h>
#include <board.h>

class FractalGIC : public InterruptSource {
public:
    virt_t gicd_base, gicc_base;

    /*
     * FractalGIC
     * Construct a new FractalGIC at a given
     * virtual base address.
     *
     * dbase: The virtual address of the GIC Distributor
     *        for this system.
     *
     * cbase: The virtual address of the per-core GIC
     *        for this core.
     */
    FractalGIC(virt_t dbase, virt_t cbase);

    virtual void Init() override;
    virtual InterruptID ClaimInterrupt() override;
    virtual void SendEOI(InterruptID) override;
};

#define GICD_CTLR(dbase)      ((dbase))
#define GICD_ISENABLER(dbase) ((dbase + 0x0100))
#define GICD_ISPENDR(dbase)   ((dbase + 0x0200))
#define GICD_ICPENDR(dbase)   ((dbase + 0x0280))
#define GICD_ISACTIVER(dbase) ((dbase + 0x0300))
#define GICD_ITARGETSR(dbase) ((dbase + 0x0800))

#define GICC_CTLR(cbase)   ((cbase))
#define GICC_PMR(cbase)    ((cbase + 0x0004))
#define GICC_IAR(cbase)    ((cbase + 0x000C))
#define GICC_EOIR(cbase)   ((cbase + 0x0010))

#define GICD_ISENABLER_IRQS_PER_REG 32
#define GICD_ITARGETSR_IRQS_PER_REG 4
#define GICD_ITARGET_CORE0 0x01

// IRQ's below this are PPI's (per-core), and above are SPI's (shared)
// SPI's need to be told which core(s) to go to via ITARGETSR.
#define GICD_FIRST_SPI_IRQNUM 32

#define GIC_PRIORITY_ALL ((0xFF))

#endif // GIC_H

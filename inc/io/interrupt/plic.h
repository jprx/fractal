#ifndef PLIC_H
#define PLIC_H

#include <types.h>
#include <interrupt.h>
#include <io/interrupt_source.h>
#include <virt_mem.h>

BEGIN_C_HEADER

extern void plic_eoi();
extern u32 plic_getirq();
extern "C" u8 uart_getc();

class FractalPLIC : public InterruptSource {
public:
    virt_t baseaddr;

    /*
     * FractalPLIC
     * Construct a new FractalPLIC at a given
     * virtual base address.
     */
    FractalPLIC(virt_t);

    virtual void Init() override;
    virtual InterruptID ClaimInterrupt() override;
    virtual void SendEOI(InterruptID) override;
};

#define PLIC_DEFAULT_BASEADDR (((0xc000000) | (KERNEL_DEVMEM_VIRT_MASK)))

#define PLIC_ENABLE_OFFSET   ((0x2000))
#define PLIC_ENABLE_SIZE     ((0x80))
#define PLIC_PRIORITY_OFFSET ((0x200000))
#define PLIC_PRIORITY_SIZE   ((0x1000))

#define PLIC_INT_PRIORITY(base)     (((base)))
#define PLIC_ENABLE(base,context)   (((base) + (PLIC_ENABLE_OFFSET) + (context * PLIC_ENABLE_SIZE)))
#define PLIC_PRIORITY(base,context) (((base) + (PLIC_PRIORITY_OFFSET) + (context * PLIC_PRIORITY_SIZE)))

#define PLIC_MCONTEXT(hartid) ((((2 * (hartid))) + 0))
#define PLIC_SCONTEXT(hartid) ((((2 * (hartid))) + 1))

// Offsets into the priority array by casting PLIC_PRIORITY to a u32*
#define PLIC_PRIORITY_THRESHOLD            ((0))
#define PLIC_PRIORITY_CLAIM_COMPLETE       ((1))

#define PLIC_NUM_INTERRUPTS 1024

END_C_HEADER

#endif // PLIC_H

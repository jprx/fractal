#ifndef INTERRUPT_CONTROLLER_H
#define INTERRUPT_CONTROLLER_H

#include <types.h>
#include <interrupt.h>

class InterruptSource {
public:
    /*
     * Init
     * Initialize the device and enable all interrupts.
     */
    virtual void Init() = 0;

    /*
     * ClaimInterrupt
     * This is called whenever an interrupt is raised by
     * this interrupt controller. This should tell the
     * interrupt controller that the core calling
     * ClaimInterrupt is servicing the interrupt.
     *
     * The interrupt ID that was raised by the interrupt
     * controller source is returned. This ID will be
     * sent back to this InterruptSource when SendEOI
     * is called.
     */
    virtual InterruptID ClaimInterrupt() = 0;

    /*
     * SendEOI
     * This is called upon completion of an interrupt that
     * began with a call to ClaimInterrupt.
     *
     * Marks the end of this interrupt ID. We return
     * the same constant that was passed to us when we called
     * ClaimInterrupt.
     */
    virtual void SendEOI(InterruptID) = 0;
};


#endif // INTERRUPT_CONTROLLER_H

#ifndef IOAPIC_H
#define IOAPIC_H

#include <types.h>

class IOAPIC {
public:
    // IO Register select
    u32 ioregsel;

    u32 _reserved0;
    u32 _reserved1;
    u32 _reserved2;

    // IO window (data)
    u32 iowin;

// Methods for this class- NO VIRTUAL METHODS ALLOWED
// (as virtual methods add a vtable)
public:
    void WriteReg(u32 reg_addr, u32 reg_val);
    u32 ReadReg(u32 reg_addr);

    // Get the IOAPIC register ID for the given irq (lower 32 bits)
    u32 RedirectionRegLower(u32 irq);

    // Get the IOAPIC register ID for the given irq (upper 32 bits)
    u32 RedirectionRegUpper(u32 irq);

    // Choose which core receives a given IRQ
    // In physical mode this is a LAPIC ID
    // In virtual mode this could be a group of processors
    void SetLAPICForIRQ(u32 irq, u8 lapic_id);

    void InitializeIRQ(u32 irq);
    void DisableIRQ(u32 irq);
    void EnableIRQ(u32 irq);
    void SetIRQ(u32 irq, u32 vector);
    u32 GetID();

    // Bringup the IOAPIC and mask all interrupts
    void Setup();
};

#define IOAPIC_REG_ID           0
#define IOAPIC_REG_VERSION      1

/// Each IOAPIC has 24 entries in its redirection table
#define IOAPIC_REDIRECTION_TABLE_SIZE 24

// Interrupt Mask is bit 16
#define IOAPIC_REDIR_MASK_INTERRUPT ((1 << 16))

// The bits that make up the destination vector for an IRQ in the redir table:
#define IOAPIC_VECTOR_MASK ((0x0FF))

#endif // IOAPIC_H

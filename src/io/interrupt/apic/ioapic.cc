#include <fractal.h>
#include <out.h>
#include <virt_mem.h>
#include <io/interrupt/ioapic.h>
#include <arch/x86.h>

u32 IOAPIC::ReadReg(u32 reg_addr) {
    *(volatile u32*)(&this->ioregsel) = reg_addr;
    return *(volatile u32*)(&this->iowin);
}

void IOAPIC::WriteReg(u32 reg_addr, u32 reg_val) {
    *(volatile u32*)(&this->ioregsel) = reg_addr;
    *(volatile u32*)(&this->iowin) = reg_val;
}

// Get the IOAPIC register ID for the given irq (lower 32 bits)
u32 IOAPIC::RedirectionRegLower(u32 irq) {
    return 0x10 + (2 * irq);
}

// Get the IOAPIC register ID for the given irq (upper 32 bits)
u32 IOAPIC::RedirectionRegUpper(u32 irq) {
    return 0x10 + (2 * irq) + 1;
}

// Choose which core receives a given IRQ
// In physical mode this is a LAPIC ID
// In virtual mode this could be a group of processors
void IOAPIC::SetLAPICForIRQ(u32 irq, u8 lapic_id) {
    u32 reg = RedirectionRegUpper(irq);
    u32 reg_val = (lapic_id) << 24;
    WriteReg(reg, reg_val);
}

// Reset all bits in this IRQ redirection table to LAPIC 0
// interrupt disabled and set to fixed mode
void IOAPIC::InitializeIRQ(u32 irq) {
    u32 reg_low = RedirectionRegLower(irq);
    WriteReg(reg_low, IOAPIC_REDIR_MASK_INTERRUPT); // Disabled
    u32 reg_hi = RedirectionRegUpper(irq);
    WriteReg(reg_hi, 0);
}

// Turn off a specific IRQ
void IOAPIC::DisableIRQ(u32 irq) {
    u32 reg = RedirectionRegLower(irq);
    u32 reg_val = ReadReg(reg) | IOAPIC_REDIR_MASK_INTERRUPT;
    WriteReg(reg, reg_val);
}

/// Turn on a specific IRQ
void IOAPIC::EnableIRQ(u32 irq) {
    u32 reg = RedirectionRegLower(irq);
    u32 reg_val = ReadReg(reg) & ~IOAPIC_REDIR_MASK_INTERRUPT;
    WriteReg(reg, reg_val);

#ifndef QUIET_BOOT
    printf("[ioapic] setting redir reg for int %d to 0x%X\r\n", irq, reg_val);
#endif // !QUIET_BOOT
}

void IOAPIC::SetIRQ(u32 irq, u32 vector) {
    u32 reg = RedirectionRegLower(irq);
    u32 reg_val = (ReadReg(reg) & ~IOAPIC_VECTOR_MASK) | vector;
    WriteReg(reg, reg_val);
}

u32 IOAPIC::GetID() {
    return ReadReg(IOAPIC_REG_ID);
}

void IOAPIC::Setup() {
#ifndef QUIET_BOOT
    printf("IOAPIC ID      0x%X\r\n", ReadReg(IOAPIC_REG_ID));
    printf("IOAPIC Version 0x%X\r\n", ReadReg(IOAPIC_REG_VERSION));
#endif // !QUIET_BOOT

    for (usize i = 0; i < IOAPIC_REDIRECTION_TABLE_SIZE; i++) {
        InitializeIRQ(i);
    }
}
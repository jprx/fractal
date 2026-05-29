#ifndef LAPIC_H
#define LAPIC_H

#include <types.h>
#include <virt_mem.h>

class LAPIC {
public:
    void Enable();
    void CheckConfiguration();
    u32 ReadReg(u32 reg);
    void WriteReg(u32 reg, u32 val);
    void SendEOI();
    void EnableTimer();
};

// Default location of the LAPIC (same for all cores)
// See AMD System Programmer's Manual Vol 2 Page 604 (16.3.1: LAPIC Enable)
#define LAPIC_DEFAULT_ADDRESS ((KERN_P2DV(0xFEE00000)))

// This is the APIC configuration register space (pointed to by the APIC BAR MSR)
// See AMD System Programmer's Manual Volume 2 Table 16-2
// These are all offsets into the 4KiB APIC configuration space
// Each register's index is their hex offset divided by sizeof(u32) == 4
// These registers *MUST* be read using aligned DWORD accesses, otherwise you
// will just see 0's. You also can't read them from GDB.
// LVT == "Local Vector Table" (AKA an interrupt source for LAPIC)
#define LAPIC_ID                        0x08
#define LAPIC_VERSION                   0x0C
#define LAPIC_TASK_PRIORITY             0x20
#define LAPIC_ARBITRATION_PRIORITY      0x24
#define LAPIC_PROCESSOR_PRIORITY        0x28
#define LAPIC_EOI                       0x2C
#define LAPIC_REMOTE_READ               0x30
#define LAPIC_LOGICAL_DEST              0x34
#define LAPIC_DEST_FORMAT               0x38
#define LAPIC_SPURIOUS_INT_VECTOR       0x3C

#define LAPIC_IRR                       0x80

#define LAPIC_ESR                       0xA0

#define LAPIC_INT_CMD_LOW               0xC0
#define LAPIC_INT_CMD_HIGH              0xC4
#define LAPIC_TIMER_LVT                 0xC8

#define LAPIC_LINT0                     0xD4
#define LAPIC_LINT1                     0xD8

#define LAPIC_LVT_ERROR                 0xDC

#define LAPIC_TIMER_INITIAL_COUNT       0xE0
#define LAPIC_TIMER_CURRENT_COUNT       0xE4
#define LAPIC_DIVIDE_CONFIG_REG         0xF8

// DIV[2] is bit 3, DIV[1:0] are bits 1:0
// This sets DIV to 1, AKA 1:1
#define LAPIC_DIVIDE_CONFIG_1TO1        0xB

#define LAPIC_TIMER_LVT_PERIODIC        ((1ull << 17ull))

// There are more, add them as you need them...

#define APIC_SPURIOUS_INT_VECTOR_ENABLE_BIT 8
#define APIC_LINT0_DISABLE_BIT              16

#endif // LAPIC_H

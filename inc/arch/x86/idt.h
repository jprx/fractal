#ifndef IDT_H
#define IDT_H

#include <fractal.h>
#include <arch/x86/selector.h>
#include <arch/x86/gdt.h>

#define IDT_NUM_ENTRIES 256

void InitIDT();

typedef struct __attribute__((packed)) IDTPointer_t {
    u16 limit;
    u64 addr;
} IDTPointer;

/*
 * | P(1) | DPL(2) | Always 0 (1) | Type (4) | Always 0 (5) | IST (3) |
 */
typedef u16 IDTEntryOptions;

#define IDT_OPTION_TYPE_MASK ((0x0F00))
#define IDT_OPTION_TYPE_SHIFT 8
#define IDT_OPTION_TYPE_INT_GATE  ((0xE << IDT_OPTION_TYPE_SHIFT))
#define IDT_OPTION_TYPE_TRAP_GATE ((0xF << IDT_OPTION_TYPE_SHIFT))

#define IDT_OPTION_PRESENT_MASK ((0x8000))
#define IDT_OPTION_PRESENT ((1 << 15ull))

#define IDT_OPTION_DPL_MASK ((0x6000))
#define IDT_OPTION_DPL_SHIFT ((13))

#define IDT_OPTION_DEFAULT ((IDT_OPTION_PRESENT | IDT_OPTION_TYPE_INT_GATE))

typedef struct __attribute__((packed)) IDTEntry_t {
public:
    u16 addr_low;
    Selector selector;
    IDTEntryOptions options;
    u16 addr_mid;
    u32 addr_high;
    u32 reserved;

    static IDTEntry_t New();
    void set_addr (u64 new_addr);
    void enable_interrupts();
    void disable_interrupts();
    void set_present(bool present);
    void set_dpl(Selector dpl);
} IDTEntry;

#endif // IDT_H

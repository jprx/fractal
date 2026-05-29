#ifndef GDT_H
#define GDT_H

#include <types.h>
#include <arch/x86/selector.h>

BEGIN_C_HEADER

typedef u16 Selector;

typedef u64 GDTEntry;

typedef struct __attribute__((packed)) {
    u16 limit;
    u64 addr;
} GDTPointer;

#define GDT_ENTRY_UNUSED 0
#define GDT_ENTRY_PRESENT   ((0x0000800000000000ull))
#define GDT_ENTRY_LONG      ((0x0020000000000000ull))
#define GDT_ENTRY_CODE_SEG  ((0x0000180000000000ull))
#define GDT_ENTRY_DATA_SEG  ((0x0000120000000000ull))

#define GDT_ENTRY_DPL_SHIFT ((45ull))

#define GDT_ENTRY_DEFAULT ((GDT_ENTRY_PRESENT | GDT_ENTRY_LONG))

#define GDT_ENTRY_CODE_KERNEL ((GDT_ENTRY_DEFAULT | GDT_ENTRY_CODE_SEG | ((DPL_KERNEL << GDT_ENTRY_DPL_SHIFT))))
#define GDT_ENTRY_CODE_USER ((GDT_ENTRY_DEFAULT | GDT_ENTRY_CODE_SEG | ((DPL_USER << GDT_ENTRY_DPL_SHIFT))))

#define GDT_ENTRY_DATA ((GDT_ENTRY_DEFAULT | GDT_ENTRY_DATA_SEG))
#define GDT_ENTRY_DATA_USER ((GDT_ENTRY_DATA | ((DPL_USER << GDT_ENTRY_DPL_SHIFT))))

/*
 * InitGDT
 * Loads GDT from higher-half world.
 */
void InitGDT();

// Reload GDT and all segment selectors using a far return
void reload_gdt(volatile void *);

#define GDT_NUM_ENTRIES 9
extern GDTEntry gdt[GDT_NUM_ENTRIES];

END_C_HEADER

#endif // GDT_H

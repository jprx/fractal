#include <arch/x86/gdt.h>
#include <page.h>

GDTEntry __attribute__((aligned(SMALL_PAGE_SIZE))) gdt[GDT_NUM_ENTRIES];

void InitGDT() {
    gdt[0] = GDT_ENTRY_UNUSED;
    gdt[SELECTOR_TO_GDT_INDEX(KERNEL_CS)] = GDT_ENTRY_CODE_KERNEL;
    gdt[SELECTOR_TO_GDT_INDEX(KERNEL_DS)] = GDT_ENTRY_DATA;
    gdt[SELECTOR_TO_GDT_INDEX(TSS_SELECTOR)] = GDT_ENTRY_UNUSED;
    gdt[SELECTOR_TO_GDT_INDEX(TSS_SELECTOR)+1] = GDT_ENTRY_UNUSED;
    gdt[SELECTOR_TO_GDT_INDEX(USER_CS_COMPAT)] = GDT_ENTRY_UNUSED;
    gdt[SELECTOR_TO_GDT_INDEX(USER_DS)] = GDT_ENTRY_DATA_USER;
    gdt[SELECTOR_TO_GDT_INDEX(USER_CS)] = GDT_ENTRY_CODE_USER;

    volatile GDTPointer p = {
        .limit = (GDT_NUM_ENTRIES * sizeof(GDTEntry) - 1),
        .addr = (usize)&gdt,
    };

    reload_gdt(&p);
}
#include <fractal.h>
#include <arch/x86/tss.h>
#include <arch.h>
#include <page.h>
#include <lib/mem.h>
#include <out.h>

TSS __attribute__((aligned(SMALL_PAGE_SIZE))) tss;

/*
 * InitTSS
 * The TSS tells the kernel where to set rsp to when
 * we take an interrupt (as it has to push the iret
 * context somewhere).
 *
 * If Fractal is running in one kernel stack per CPU
 * mode, this value can be hardcoded to the bottom of
 * the link memory address of the "single kernel stack".
 *
 * Otherwise, tss.rsp0 is updated forcefully by task.cc
 * when running in 1 kernel stack-per-task mode.
 */
void InitTSS() {
    tss.rsp0 = KERNEL_STACK;
#ifndef QUIET_BOOT
    printf("tss.rsp0: 0x%X\r\n", tss.rsp0);
#endif // !QUIET_BOOT
    tss.iomap_base_addr = -1;

    TSSDescriptor tss_desc = TSSDescriptor::New();
    tss_desc.set_addr((usize)&tss);
    tss_desc.set_limit(sizeof(tss));

    memcpy(&gdt[SELECTOR_TO_GDT_INDEX(TSS_SELECTOR)], &tss_desc, sizeof(tss_desc));

    asm volatile(
        "ltr %0"
        :: "r"(TSS_SELECTOR)
    );
}

TSSDescriptor TSSDescriptor::New() {
    return TSSDescriptor {
        .limit_low = 0,
        .addr_low = 0,
        .addr_mid1 = 0,
        .options = TSS_OPTION_DEFAULT,
        .limit_high = 0,
        .addr_mid2 = 0,
        .addr_high = 0,
        .res0 = 0,
    };
}

void TSSDescriptor::set_addr(u64 new_addr) {
    addr_low = new_addr & 0x0FFFF;
    addr_mid1 = (new_addr >> 16) & 0x0FF;
    addr_mid2 = (new_addr >> 24) & 0x0FF;
    addr_high = (new_addr >> 32) & 0x0FFFFFFFF;
}

void TSSDescriptor::set_limit(u64 new_limit) {
    limit_low = new_limit & 0x0FFFF;
    limit_high = (new_limit >> 16) & 0x07;
}

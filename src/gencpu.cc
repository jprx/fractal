#include <cpu.h>
#include <arch.h>
#include <types.h>
#include <lib/mem.h>
#include <page_alloc.h>
#include <virt_mem.h>
#include <out.h>
#include <page.h>
#include <task.h>
#include <syscall.h>
#include <stdio.h>
#include <paging.h>

#define BITS_PER_INDIRECTION_LEVEL 9ull
#define OFFSET_BITS 12ull
#define N_PAGETABLE_LEVELS 4ull
#define LAST_LEVEL ((N_PAGETABLE_LEVELS - 1))

/*
 * GENCPU
 * A generic cross-platform CPU implementation.
 *
 * These are operations common to every architecture (eg.
 * mapping memory, performing a page table walk, etc.) that
 * are written to be platform-independent.
 *
 * Derived classes must implement various methods to support
 * these, but at a high level these methods are provided
 * to eliminate code duplication and repetition throughout
 * the kernel (and hopefully reducing bugs along the way).
 */

// The default MapKernel method- map the kernel into this page table.
// ARM overrides this because it has a separate physical page table for the kernel.
void FractalCore::MapKernel(u64 *pt) {
    // Map kernel as normal memory
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        PageMap(
            pt,
            KERNEL_LINK_ADDRESS + (i * ONE_GB),
            KERN_V2P(KERNEL_LINK_ADDRESS + (i * ONE_GB)),
            GIANT_PAGE,
            KERNEL_PAGE
        );
    }

    // Map devices as device memory
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        PageMap(
            pt,
            KERNEL_DEVICEMAP_BEGIN + (i * ONE_GB),
            KERN_DV2P(KERNEL_DEVICEMAP_BEGIN + (i * ONE_GB)),
            GIANT_PAGE,
            KERNEL_PAGE
        );

        SetPageMemoryKind(
            pt,
            KERNEL_DEVICEMAP_BEGIN + (i * ONE_GB),
            MEMORY_UNCACHEABLE
        );
    }

    // Map framebuffer as write-combining memory
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        PageMap(
            pt,
            KERNEL_WRITECOMB_BEGIN + (i * ONE_GB),
            KERN_WCV2P(KERNEL_WRITECOMB_BEGIN + (i * ONE_GB)),
            GIANT_PAGE,
            KERNEL_PAGE
        );

        SetPageMemoryKind(
            pt,
            KERNEL_WRITECOMB_BEGIN + (i * ONE_GB),
            MEMORY_WRITECOMB
        );
    }
}

void FractalCore::UnmapKernel(u64 *pt) {
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        // Unmap normal kernel map
        PageUnmap(
            pt,
            KERNEL_LINK_ADDRESS + (i * ONE_GB)
        );

        // Unmap device map
        PageUnmap(
            pt,
            KERNEL_DEVICEMAP_BEGIN + (i * ONE_GB)
        );

        // Unmap write-combining map
        PageUnmap(
            pt,
            KERNEL_WRITECOMB_BEGIN + (i * ONE_GB)
        );
    }
}

void FractalCore::InitializePaging() {
    return;
}

void FractalCore::TimerInterrupt() {
    naptime();
}

kret_t FractalCore::DetectHardwareFramebuffer() {
    return ALL_GOOD;
}

kret_t FractalCore::DetectPlatformIO() {
    return ALL_GOOD;
}

void FractalCore::InitializeCaching() {
    return;
}

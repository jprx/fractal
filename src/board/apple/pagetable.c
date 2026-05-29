#include <fractal.h>
#include <board/arm/apple/soc.h>
#include <kasan.h>

#ifndef BOARD_APPLE
#error "Compiling pagetable.c for a non-Apple target!"
#endif // ! BOARD_APPLE

#define NUM_ENTRIES_4K     512

#define BLOCK_ENTRY        0x401
#define TABLE_DESCRIPTOR   0x03
#define REMOVE_ENTRYDATA   ((0xFFFFFFFFFFFFF000ull))

// 1 L0 page table that points to 2 L1 page tables (1 for UART, 1 for kernel)
// Each L1 page table *should* be 512 1GB giant pages, but Apple Silicon doesn't support that.
// Instead, each L1 page table points to a L2 page table full of 2MB pages.

// This code is going in .text.start_c, so we cannot use anything in bss, rodata, etc.
// It also runs before paging is enabled (obviously), so no calling any other functions
// in Fractal, as things like strcpy are in the higher half and are unmapped currently.
#define TEXT_START __attribute__((section(".text.start_c")))

#define PT_IDX_MASK ((0x01FF))
#define TO_L0_IDX(v) (((v >> 39ull) & PT_IDX_MASK))
#define TO_L1_IDX(v) (((v >> 30ull) & PT_IDX_MASK))
#define TO_L2_IDX(v) (((v >> 21ull) & PT_IDX_MASK))
#define HIGHER_PHYS_BITS(v) (((v & ~MASK(30ull))))
#define L0_HIGHER_HALF ((NUM_ENTRIES_4K >> 1ull))

// The size of pg_tables
#define MAX_PAGES_TO_USE ((16))

#define MAIR_NORMAL ((0b000 << 2))
#define MAIR_DEVICE ((0b001 << 2))

static inline TEXT_START KASAN_IGNORE u64 *alloc_page(u64 *pg_tables, u64 *num_used_pages) {
    u64 *rval = &pg_tables[(*num_used_pages) * NUM_ENTRIES_4K];
    (*num_used_pages)++;

    // No panic() support yet, far too early
    if (*num_used_pages > MAX_PAGES_TO_USE) while(1);

    return rval;
}

// Identity maps 1GB based at vaddr, allocating pages as necessary
// Device memory has MAIR index 1; normal memory has index 0
static inline TEXT_START KASAN_IGNORE void mapin(u64 vaddr, u64 mair, u64 *l0, u64 *pg_tables, u64 *num_used_pages) {
#define ALLOC_PAGE() alloc_page(pg_tables, num_used_pages)
    u64 *l1, *l2;

    if (!l0[TO_L0_IDX(vaddr)]) l0[TO_L0_IDX(vaddr)] = (u64)ALLOC_PAGE() | TABLE_DESCRIPTOR;
    l1 = (u64*)(l0[TO_L0_IDX(vaddr)] & REMOVE_ENTRYDATA);
    if (!l1[TO_L1_IDX(vaddr)]) l1[TO_L1_IDX(vaddr)] = (u64)ALLOC_PAGE() | TABLE_DESCRIPTOR;
    l2 = (u64*)(l1[TO_L1_IDX(vaddr)] & REMOVE_ENTRYDATA);

    for (int i = 0; i < NUM_ENTRIES_4K; i++) {
        l2[i] = HIGHER_PHYS_BITS(vaddr) | (i << 21ull) | BLOCK_ENTRY | mair;
    }

    l0[TO_L0_IDX(vaddr) | L0_HIGHER_HALF] = l0[TO_L0_IDX(vaddr)];
}

// pg_tables: pointer to MAX_PAGES_TO_USE 4K aligned pages in physical addr space
TEXT_START KASAN_IGNORE void apple_board_mk_pagetable(u64 *pg_tables, u64 fractal_start_phys) {
    u64 num_used_pages = 1;
    u64 *l0  = &pg_tables[0];
    mapin(APPLE_SERIAL_BASEADDR,             MAIR_DEVICE, l0, pg_tables, &num_used_pages);
    mapin(fractal_start_phys + (0 * ONE_GB), MAIR_NORMAL, l0, pg_tables, &num_used_pages);
    mapin(fractal_start_phys + (1 * ONE_GB), MAIR_NORMAL, l0, pg_tables, &num_used_pages);
    mapin(fractal_start_phys + (2 * ONE_GB), MAIR_NORMAL, l0, pg_tables, &num_used_pages);
    mapin(fractal_start_phys + (3 * ONE_GB), MAIR_NORMAL, l0, pg_tables, &num_used_pages);
    mapin(fractal_start_phys + (4 * ONE_GB), MAIR_NORMAL, l0, pg_tables, &num_used_pages);
}

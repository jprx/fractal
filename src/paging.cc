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
#include <fmap.h>

/*
 * A 48 bit virtual address looks like this:
 * |63    48|47     39|38     30|29     21|20     12|11     0|
 * +---------------------------------------------------------+
 * |  Sign  | Level 4 | Level 3 | Level 2 | Level 1 | Offset |
 * |        | (SUPER) | (GIANT) | (LARGE) | (SMALL) |        |
 * +---------------------------------------------------------+
 *
 * L1 indexes small pages (4KB)
 * L2 indexes large pages (2MB)
 * L3 indexes giant pages (1GB)
 * L4 indexes super pages (512GB)
 */

// #define GMAP_LOGGING 1

#define GIGAMAP_NUM_L4_ENTRIES 32

static inline usize get_pt_index(virt_t va, pagelevel_t level) {
    return (va >> (PAGE_OFFSET_BITS + (PAGE_BITS_PER_INDIRECTION_LEVEL * level))) & ((BIT(PAGE_BITS_PER_INDIRECTION_LEVEL)) - 1);
}

static inline usize get_page_offset(virt_t va, pagelevel_t level) {
    return va & (BIT((PAGE_BITS_PER_INDIRECTION_LEVEL * level) + PAGE_OFFSET_BITS) - 1);
}

static inline bool is_aligned_to_level(virt_t va, pagelevel_t level) {
    return 0 == get_page_offset(va, level);
}

/**
 * PageMap
 * Maps a given virt -> phys mapping into a given page table
 *
 * Arguments:
 * - page_table: A pointer to the top level of a page table into which to map the page
 * - virt: The virtual address of the page
 * - phys: The physical address of the page
 * - sz: The size of the page (SMALL_PAGE, LARGE_PAGE, or GIANT_PAGE)
 * - priv: The privilege level at which to map this page
 */
kret_t PageMap(u64 *page_table, virt_t virt, phys_t phys, PageSize sz, PagePrivilege priv) {
    FRACTALPTE_CLASS cur_entry, new_entry;
    u64 *prev_page_table = (u64 *)page_table;

    if (!page_table) return THING_DOESNT_EXIST;

    usize last_level = PAGE_L1;
    switch(sz) {
        case SMALL_PAGE:
        last_level = PAGE_L1;
        break;

        case LARGE_PAGE:
        last_level = PAGE_L2;
        break;

        case GIANT_PAGE:
        last_level = PAGE_L3;
        break;

        default:
        panic("PageMap: tried to map a page of unsupported size (%d)", sz);
        break;
    }

    for (usize level = PAGE_L4; level >= last_level; level--) {
        usize page_index = get_pt_index(virt, (pagelevel_t)level);

        cur_entry.Clear();
        cur_entry.From(prev_page_table[page_index]);

        if (last_level == level) {
            // End of walk- This is the entry to fill in
            new_entry.Clear();
            new_entry.SetPhysAddr(KERN_V2P(phys));
            new_entry.SetSize(sz);
            new_entry.SetLeafPage(priv);
            prev_page_table[page_index] = new_entry.Serialize();
            return ALL_GOOD;
        }

        // Continuing walk- update intermediate page tables accordingly
        usize next_page_table = 0;
        if (!(cur_entry.IsPresent())) {
            // Miss- fill in new page pointer here
            next_page_table = AllocPage(SMALL_PAGE);
            memset((u8 *)next_page_table, '\x00', SMALL_PAGE_SIZE);
        } else {
            // Greedily steal the next level page table
            next_page_table = KERN_P2V(cur_entry.GetPhysAddr());
        }

        // @TODO: This may trigger a memory leak if you remapped a small page into a large one,
        // the inner page table is left dangling as we replaced it without freeing it. So, @TODO: not that.
        new_entry.Clear();
        new_entry.SetPhysAddr(KERN_V2P(next_page_table));
        new_entry.SetIntermediatePage();
        prev_page_table[page_index] = new_entry.Serialize();

        prev_page_table = (u64 *)KERN_P2V(new_entry.GetPhysAddr());
    }

    panic("PageMap: shouldn't ever reach here");
}

/**
 * _unmap_recursive
 * A recursive method that iterates down into a virtual address through the page table levels.
 * We perform the following check at each level:
 *   1. If we are the last level (or a leaf), mark the page table entry as not present
 *      If not at the end, then this entry points to another page table- recurse into it.
 *      If that recursive call tells us we freed the entire page table beneath us (by returning true),
 *      we need to mark the current entry as not present as well.
 *   2. Now that the current entry is marked free if it needs to be, scan the entire page table.
 *      If there are no other present entries, we need to free it as well.
 *      If we find a single present entry, return false to indicate to the caller we didn't free this page.
 *   3. If we did not return false, then we free the page and return true to indicate to the next level that
 *      this page was freed.
 */
bool _unmap_recursive(u64 *cur_pagetable, virt_t virt, usize level) {
    FRACTALPTE_CLASS cur_entry;
    usize page_index = get_pt_index(virt, (pagelevel_t)level);

    cur_entry.Clear();
    cur_entry.From(cur_pagetable[page_index]);

    if (!cur_entry.IsPresent()) {
        // In the future this will be an error return code, not a panic
        panic("Tried to unmap a non-present page");
    }

    // 1. If this is the last level, free the entry in the current page table
    // Otherwise, recurse down into the page table
    if (PAGE_L1 == level || (cur_entry.IsPresent() && cur_entry.IsLeaf())) {
        // We are the last level, free this entry and don't recurse any more
        cur_entry.SetPresent(false);
        cur_pagetable[page_index] = cur_entry.Serialize();
    } else {
        // Not the last level, recurse down into the current entry
        virt_t *next_pagetable = (virt_t *)KERN_P2V(cur_entry.GetPhysAddr());
        bool freed_entire_next_page = _unmap_recursive(next_pagetable, virt, level - 1);

        if (freed_entire_next_page) {
            // Recursive call freed the page beneath us, so set it to not present in this page
            cur_entry.SetPresent(false);
            cur_pagetable[page_index] = cur_entry.Serialize();
        }
    }

    // 2. Check if the page table has any other present pages
    // If not, completely free it
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        cur_entry.Clear();
        cur_entry.From(cur_pagetable[i]);
        if (cur_entry.IsPresent()) {
            // There's something else in this page, we are done here- leave
            return false;
        }
    }

    // 3. If we didn't return in step 2, then this page table is ready to be freed
    FreePage((virt_t)cur_pagetable);
    return true;
}

/**
 * PageUnmap
 * Unmaps a virtual address from a page table, freeing all allocated pages as we go.
 * Undoes Map(). Leaves the physical pages in-tact, just cleans them up from the page table.
 *
 * Arguments:
 * - page_table: A pointer to the top level of a page table from which we will free the page
 * - virt: The virtual address to free
 *
 * Return Value:
 * If the entire page table has been freed, returns true. Otherwise, returns false.
 */
bool PageUnmap(u64 *page_table, virt_t virt) {
    bool freed_whole_page = _unmap_recursive(page_table, virt, PAGE_L4);
    if (freed_whole_page) {
        return true;
    }
    return false;
}

// Fill in a L3 or L2 page table for the gigamap
// (all unused entries point to the next table in the gigamap)
void _gmap_fillin_new_pt(gigamap_t *gmap, usize level, u64 *new_pt) {
    FRACTALPTE_CLASS entry;
    entry.Clear();
    switch(level) {
        case PAGE_L3:
        if (0 == gmap->g_l2_pagetable) panic("tried to fill in an l3 gmap page table but no l2 exists");
        entry.SetPhysAddr(KERN_V2P((usize)gmap->g_l2_pagetable));
        entry.SetIntermediatePage();
        break;

        case PAGE_L2:
        if (0 == gmap->g_map_pa) panic("tried to fill in an l2 gmap page table but no PA has been assigned");
        entry.SetPhysAddr(KERN_V2P(gmap->g_map_pa));
        entry.SetExtraBits((u8)DUPLICATED_PAGE); // base PA is always considered duplicated
        entry.SetSize(LARGE_PAGE);
        entry.SetLeafPage(gmap->g_privs);
        break;

        default:
        panic("_gmap_fillin_new_pt: invalid level requested (%d)", level);
        break;
    }

    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        new_pt[i] = entry.Serialize();
    }
}

kret_t GigaMap(gigamap_t *gmap, u64 *page_table, virt_t virt, virt_t backing_va, PagePrivilege privs) {
    FRACTALPTE_CLASS entry;
    const usize l4_baseidx = get_pt_index(virt, PAGE_L4);

    if (!page_table) return THING_DOESNT_EXIST;
    if (!gmap) return THING_DOESNT_EXIST;
    if (gmap->g_is_mapped) return OUT_OF_BOUNDS;
    entry.Clear();

    // virt should be level 4 aligned
    if (!is_aligned_to_level(virt, PAGE_L4)) {
        printf("[GigaMap] alignment error at bit 39 (0x%X)\n", virt);
        return OUT_OF_BOUNDS;
    }

    // nothing should be present in the region we're going to map over
    for (usize i = 0; i < GIGAMAP_NUM_L4_ENTRIES; i++) {
        usize l4_idx = i + l4_baseidx;
        entry.Clear();
        entry.From(page_table[l4_idx]);

        if (entry.IsPresent()) {
            printf("[GigaMap] something is already mapped on top of the giga page\n");
            return OUT_OF_BOUNDS;
        }
    }

    if (PHYS_ADDR_INVALID != PageWalk(page_table, backing_va)) {
#ifdef GMAP_LOGGING
        printf("[gmap] something is already mapped at the base VA");
#endif // GMAP_LOGGING
        return OUT_OF_BOUNDS;
    }

    virt_t new_pg = AllocPage(LARGE_PAGE);
    if ((virt_t)0 == new_pg) {
        panic("GigaMap: failed to allocate memory");
    }

    gmap->g_map_va = virt;
    gmap->g_map_pa = KERN_V2P(new_pg);
    gmap->g_backing_va = backing_va;
    gmap->g_privs = privs;

    gmap->g_l2_pagetable = (u64*)AllocPage(SMALL_PAGE);
    _gmap_fillin_new_pt(gmap, PAGE_L2, gmap->g_l2_pagetable);
    gmap->g_l3_pagetable = (u64*)AllocPage(SMALL_PAGE);
    _gmap_fillin_new_pt(gmap, PAGE_L3, gmap->g_l3_pagetable);

    PageMap(page_table, gmap->g_backing_va, gmap->g_map_pa, LARGE_PAGE, privs);
    SetPageExtraBits(page_table, gmap->g_backing_va, DUPLICATED_PAGE);

#ifdef GMAP_LOGGING
    printf("[gmap] Base VA: 0x%X\n", gmap->g_map_va);
    printf("[gmap] Base PA: 0x%X\n", gmap->g_map_pa);
    printf("[gmap] Allocated L2 at 0x%X\n", gmap->g_l2_pagetable);
    printf("[gmap] Allocated L3 at 0x%X\n", gmap->g_l3_pagetable);
#endif // GMAP_LOGGING

    entry.Clear();
    entry.SetPhysAddr(KERN_V2P((usize)gmap->g_l3_pagetable));
    entry.SetIntermediatePage();

    for (usize i = 0; i < GIGAMAP_NUM_L4_ENTRIES; i++) {
        usize l4_idx = i + l4_baseidx;
        page_table[l4_idx] = entry.Serialize();
    }

    gmap->g_is_mapped = true;
    global_cpu.FlushTLB();
    return ALL_GOOD;
}

// Carve out a single L2 page from the gigamap
kret_t GigaMapCarveout(gigamap_t *gmap, u64 *page_table, virt_t virt) {
    FRACTALPTE_CLASS cur_entry, new_entry;
    u64 *prev_page_table = (u64 *)page_table;

    if (!page_table) return THING_DOESNT_EXIST;
    if (!gmap) return THING_DOESNT_EXIST;
    if (!gmap->g_is_mapped) return OUT_OF_BOUNDS;

    // minimum carveout granularity is 1 large page
    virt = ALIGN_LARGE_PAGE(virt);

    if (virt < gmap->g_map_va || virt > gmap->g_map_va + (GIGAMAP_NUM_L4_ENTRIES * SUPER_PAGE_SIZE)) {

#ifdef GMAP_LOGGING
        printf("[gmap] requested carveout at out-of-bounds VA 0x%X. Bounds: 0x%X -> 0x%X\n",
               (u64)virt,
               gmap->g_map_va,
               (GIGAMAP_NUM_L4_ENTRIES * SUPER_PAGE_SIZE)
        );
#endif // GMAP_LOGGING

        return OUT_OF_BOUNDS;
    }

    if (PageWalk(page_table, virt) == PHYS_ADDR_INVALID) {
        // gigapage should have no empty maps- only the default page or a carveout
        panic("GigaMapCarveout: detected invalid mapping at 0x%X\n", virt);
    }

    // if we already carved out this VA, stop here
    if (PageWalk(page_table, virt) != gmap->g_map_pa) {
#ifdef GMAP_LOGGING
        printf("[gmap] carveout detected existing carveout at 0x%X, skipping early\n", virt);
#endif // GMAP_LOGGING
        return ALL_GOOD;
    }

#ifdef GMAP_LOGGING
    printf("[gmap] Carving out 0x%X\n", virt);
#endif // GMAP_LOGGING

    for (usize level = PAGE_L4; level >= PAGE_L2; level--) {
        usize page_index = get_pt_index(virt, (pagelevel_t)level);

        cur_entry.Clear();
        cur_entry.From(prev_page_table[page_index]);

        if (PAGE_L2 == level) {
            // End of walk- This is the entry to fill in
            virt_t new_pg = AllocPage(LARGE_PAGE);
            if ((virt_t)0 == new_pg) panic("GigaMapCarveout: couldn't allocate more memory");

#ifdef GMAP_LOGGING
            printf("[gmap] Carved out PA 0x%X\n", new_pg);
#endif // GMAP_LOGGING

            memcpy((virt_t *)new_pg, (virt_t *)KERN_P2V(gmap->g_map_pa), LARGE_PAGE_SIZE);

            new_entry.Clear();
            new_entry.SetPhysAddr(KERN_V2P(new_pg));
            new_entry.SetSize(LARGE_PAGE);
            new_entry.SetExtraBits(ALLOCATED_PAGE);
            new_entry.SetLeafPage(gmap->g_privs);
            prev_page_table[page_index] = new_entry.Serialize();
            return ALL_GOOD;
        }

        // Continuing walk- update intermediate page tables accordingly
        usize next_page_table = 0;
        u64 *next_page_table_expected = NULL;
        if (!(cur_entry.IsPresent())) {
            panic("found an unmapped PTE in the giga page");
        }

        switch(level) {
            case PAGE_L4:
                next_page_table_expected = gmap->g_l3_pagetable;
                break;

            case PAGE_L3:
                next_page_table_expected = gmap->g_l2_pagetable;
                break;

            default:
                panic("GigaMapCarveout: We should only ever see levels 4 or 3 at inner nodes");
                break;
        }

        // If the next page table is one of the gigamap default ones, map in a new one
        // Otherwise use the page that's already mapped in
        if (next_page_table_expected == (u64*)KERN_P2V(cur_entry.GetPhysAddr())) {
            next_page_table = AllocPage(SMALL_PAGE);
            _gmap_fillin_new_pt(gmap, level - 1, (u64*)next_page_table);

#ifdef GMAP_LOGGING
            printf("[gmap] Carved out intermediate L%d page table at 0x%X\n", level - 1, next_page_table);
#endif // GMAP_LOGGING

        } else {
            next_page_table = KERN_P2V(cur_entry.GetPhysAddr());
        }

        new_entry.Clear();
        new_entry.SetPhysAddr(KERN_V2P(next_page_table));
        new_entry.SetIntermediatePage();
        prev_page_table[page_index] = new_entry.Serialize();

        prev_page_table = (u64 *)KERN_P2V(new_entry.GetPhysAddr());
    }

    panic("GigaMapCarveout: shouldn't ever reach here");
}

kret_t GigaMapGetBase(gigamap_t *gmap, u64 *page_table, virt_t *outv) {
    if (!gmap) return THING_DOESNT_EXIST;
    if (!page_table) return THING_DOESNT_EXIST;
    if (!outv) return THING_DOESNT_EXIST;
    if (!gmap->g_is_mapped) return THING_DOESNT_EXIST;

    // the base page is greedily allocated during the mapping in of the gigapage
    // here we just check that the page is still what we expect it to be and return it
    phys_t currently_mapped_pa = PageWalk(page_table, gmap->g_backing_va);
    if (currently_mapped_pa != gmap->g_map_pa) {
        panic("GigaMapGetBase: currently mapped PA is not the expected PA for this gigamap");
    }

    *outv = gmap->g_backing_va;
    return ALL_GOOD;
}

// Traverse L2 table, freeing any entries that point to non-default PAs
void _gigamap_free_l2(gigamap_t *gmap, u64 *l2_table) {
    FRACTALPTE_CLASS entry;

    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        entry.Clear();
        entry.From(l2_table[i]);
        phys_t cur_pa = entry.GetPhysAddr();

        if (cur_pa != gmap->g_map_pa) {
            if (entry.GetExtraBits() != ALLOCATED_PAGE) {
                panic("found a non-allocated non-default leaf PTE in a giga map (bits: 0x%X)", entry.GetExtraBits());
            }

#ifdef GMAP_LOGGING
            printf("[gmap] Freeing base PA 0x%X\n", KERN_P2V(cur_pa));
#endif // GMAP_LOGGING

            FreePage((virt_t)KERN_P2V(cur_pa));
        }
    }
}

// Traverse L3 table, freeing any entries that point to non-default L2 tables
void _gigamap_free_l3(gigamap_t *gmap, u64 *l3_table) {
    FRACTALPTE_CLASS entry;

    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        entry.Clear();
        entry.From(l3_table[i]);
        u64 *l2_pt = (u64*)KERN_P2V(entry.GetPhysAddr());

        if (l2_pt != gmap->g_l2_pagetable) {
            _gigamap_free_l2(gmap, l2_pt);

#ifdef GMAP_LOGGING
            printf("[gmap] Freeing L2 page table 0x%X\n", l2_pt);
#endif // GMAP_LOGGING

            FreePage((virt_t)l2_pt);
        }
    }
}

kret_t GigaMapReset(gigamap_t *gmap, u64 *page_table) {
    FRACTALPTE_CLASS entry;
    if (!gmap) return THING_DOESNT_EXIST;
    if (!page_table) return THING_DOESNT_EXIST;
    if (!gmap->g_is_mapped) return THING_DOESNT_EXIST;

    for (usize i = 0; i < GIGAMAP_NUM_L4_ENTRIES; i++) {
        usize l4_idx = i + get_pt_index(gmap->g_map_va, PAGE_L4);

        entry.Clear();
        entry.From(page_table[l4_idx]);

        u64 *l3_pt = (u64*)KERN_P2V(entry.GetPhysAddr());

        if (l3_pt != gmap->g_l3_pagetable) {
            _gigamap_free_l3(gmap, l3_pt);

#ifdef GMAP_LOGGING
            printf("[gmap] Freeing L3 page table 0x%X\n", l3_pt);
#endif // GMAP_LOGGING

            FreePage((virt_t)l3_pt);
            entry.Clear();
            entry.SetPhysAddr(KERN_V2P((usize)gmap->g_l3_pagetable));
            entry.SetIntermediatePage();
            page_table[l4_idx] = entry.Serialize();
        }
    }

    return ALL_GOOD;
}

kret_t GigaMapUnmap(gigamap_t *gmap, u64 *page_table) {
    kret_t rv;
    FRACTALPTE_CLASS entry;
    if (!gmap) return THING_DOESNT_EXIST;
    if (!page_table) return THING_DOESNT_EXIST;
    if (!gmap->g_is_mapped) return THING_DOESNT_EXIST;

#ifdef GMAP_LOGGING
    printf("[gmap] Unmapping the gigapage\n");
#endif // GMAP_LOGGING

    // this will unmap any carveouts so that our gigamap is fully continuous,
    // making removal much simpler.
    rv = GigaMapReset(gmap, page_table);
    if (ALL_GOOD != rv) return rv;

    // only need to check L4 PTEs to L3 page tables, as if they all point to the same L3 table,
    // we can be sure that everything L2 and below is as expected and will be freed when we free the gmap.
    for (usize i = 0; i < GIGAMAP_NUM_L4_ENTRIES; i++) {
        usize l4_idx = i + get_pt_index(gmap->g_map_va, PAGE_L4);
        entry.Clear();
        entry.From(page_table[l4_idx]);

        if (KERN_P2V(entry.GetPhysAddr()) != (virt_t)gmap->g_l3_pagetable) {
            panic("GigaMapUnmap: found a non-default L3 PTE in the page table at L4 idx %d, but we already called reset so there should be none", l4_idx);
        }

        entry.SetPresent(false);
        page_table[l4_idx] = entry.Serialize();
    }

    if (gmap->g_map_pa != PageWalk(page_table, gmap->g_backing_va)) {
        panic("GigaMapUnmap: base pointer isn't what we expect");
    }

    if (GetPageExtraBits(page_table, gmap->g_backing_va) != DUPLICATED_PAGE) {
        panic("GigaMapUnmap: base pointer isn't marked as duplicated");
    }

    PageUnmap(page_table, gmap->g_backing_va);

#ifdef GMAP_LOGGING
    printf("Freeing L2 pagetable: 0x%X\n", gmap->g_l2_pagetable);
    printf("Freeing L3 pagetable: 0x%X\n", gmap->g_l3_pagetable);
    printf("Freeing gmap base PA: 0x%X\n", gmap->g_map_pa);
#endif // GMAP_LOGGING

    FreePage((virt_t)gmap->g_l2_pagetable);
    FreePage((virt_t)gmap->g_l3_pagetable);
    FreePage(KERN_P2V(gmap->g_map_pa));

    memset(gmap, '\x00', sizeof(*gmap));
    gmap->g_is_mapped = false;
    return ALL_GOOD;
}

/**
 * PageWalk
 * Walk a page table and return the physical address for this virtual one.
 * Returns PHYS_ADDR_INVALID if the address is not present.
 */
phys_t PageWalk(u64 *page_table, virt_t virt) {
    FRACTALPTE_CLASS cur_entry;
    u64 *prev_page_table = page_table;

    if (!is_canonical(virt)) {
        return PHYS_ADDR_INVALID;
    }

    for (usize level = PAGE_L4; level >= PAGE_L1; level--) {
        usize page_index = get_pt_index(virt, (pagelevel_t)level);

        cur_entry.Clear();
        cur_entry.From(prev_page_table[page_index]);

        if (!cur_entry.IsPresent()) {
            return PHYS_ADDR_INVALID;
        }

        if ((level == PAGE_L1) || cur_entry.IsLeaf()) {
            usize va_offset = get_page_offset(virt, (pagelevel_t)level);
            usize pa_offset = get_page_offset(cur_entry.GetPhysAddr(), (pagelevel_t)level);
            if (0 != pa_offset) panic("PageWalk: found a misaligned PA");
            return cur_entry.GetPhysAddr() | va_offset;
        }

        prev_page_table = (u64 *)KERN_P2V(cur_entry.GetPhysAddr());
    }

    return PHYS_ADDR_INVALID;
}

bool IsPagePresent(u64 *page_table, virt_t virt) {
    return PageWalk(page_table, virt) != PHYS_ADDR_INVALID;
}

void SetPageMemoryKind(u64 *page_table, virt_t virt, memory_kind_t kind) {
    FRACTALPTE_CLASS cur_entry;
    u64 *prev_page_table = page_table;

    if (!is_canonical(virt)) {
        return;
    }

    for (usize level = PAGE_L4; level >= PAGE_L1; level--) {
        usize page_index = get_pt_index(virt, (pagelevel_t)level);

        cur_entry.Clear();
        cur_entry.From(prev_page_table[page_index]);

        if (!cur_entry.IsPresent()) {
            return;
        }

        if ((level == PAGE_L1) || cur_entry.IsLeaf()) {
            switch(kind) {
                case MEMORY_WRITEBACK:   cur_entry.SetWriteback();   break;
                case MEMORY_UNCACHEABLE: cur_entry.SetUncacheable(); break;
                case MEMORY_WRITECOMB:   cur_entry.SetWritecomb();   break;
                default: panic("SetPageMemoryKind: unknown memory kind %d", kind);
            }
            prev_page_table[page_index] = cur_entry.Serialize();
            return;
        }

        prev_page_table = (u64 *)KERN_P2V(cur_entry.GetPhysAddr());
    }

    panic("SetPageMemoryKind: shouldn't ever get here");
}

void SetPageMemoryPermission(u64 *page_table, virt_t virt, PagePrivilege priv) {
    FRACTALPTE_CLASS cur_entry;
    u64 *prev_page_table = page_table;

    if (!is_canonical(virt)) {
        return;
    }

    for (usize level = PAGE_L4; level >= PAGE_L1; level--) {
        usize page_index = get_pt_index(virt, (pagelevel_t)level);

        cur_entry.Clear();
        cur_entry.From(prev_page_table[page_index]);

        if (!cur_entry.IsPresent()) {
            return;
        }

        if ((level == PAGE_L1) || cur_entry.IsLeaf()) {
            switch(priv) {
                case USER_PAGE:
                case KERNEL_PAGE:
                cur_entry.SetPrivilege(priv);
                break;

                default: panic("SetPageMemoryPermission: unknown privilege kind %d", priv);
            }
            prev_page_table[page_index] = cur_entry.Serialize();
            return;
        }

        prev_page_table = (u64 *)KERN_P2V(cur_entry.GetPhysAddr());
    }

    panic("SetPageMemoryPermission: shouldn't ever get here");
}

void SetPageExtraBits(u64 *page_table, virt_t virt, u8 extra) {
    FRACTALPTE_CLASS cur_entry;
    u64 *prev_page_table = page_table;

    if (!is_canonical(virt)) {
        return;
    }

    for (usize level = PAGE_L4; level >= PAGE_L1; level--) {
        usize page_index = get_pt_index(virt, (pagelevel_t)level);

        cur_entry.Clear();
        cur_entry.From(prev_page_table[page_index]);

        if (!cur_entry.IsPresent()) {
            return;
        }

        if ((level == PAGE_L1) || cur_entry.IsLeaf()) {
            cur_entry.SetExtraBits(extra);
            prev_page_table[page_index] = cur_entry.Serialize();
            return;
        }

        prev_page_table = (u64 *)KERN_P2V(cur_entry.GetPhysAddr());
    }

    panic("SetPageExtraBits: shouldn't ever get here");
}

u8 GetPageExtraBits(u64 *page_table, virt_t virt) {
    FRACTALPTE_CLASS cur_entry;
    u64 *prev_page_table = page_table;

    if (!is_canonical(virt)) {
        return 0;
    }

    for (usize level = PAGE_L4; level >= PAGE_L1; level--) {
        usize page_index = get_pt_index(virt, (pagelevel_t)level);

        cur_entry.Clear();
        cur_entry.From(prev_page_table[page_index]);

        if (!cur_entry.IsPresent()) {
            return 0;
        }

        if ((level == PAGE_L1) || cur_entry.IsLeaf()) {
            return cur_entry.GetExtraBits();
        }

        prev_page_table = (u64 *)KERN_P2V(cur_entry.GetPhysAddr());
    }

    panic("GetPageExtraBits failed");
    return -1;
}

usize GetPageSize(u64 *page_table, virt_t virt) {
    FRACTALPTE_CLASS cur_entry;
    u64 *prev_page_table = page_table;

    if (!is_canonical(virt)) {
        return 0;
    }

    for (usize level = PAGE_L4; level >= PAGE_L1; level--) {
        usize page_index = get_pt_index(virt, (pagelevel_t)level);

        cur_entry.Clear();
        cur_entry.From(prev_page_table[page_index]);

        if (!cur_entry.IsPresent()) {
            return 0;
        }

        if ((level == PAGE_L1) || cur_entry.IsLeaf()) {
            switch(level) {
                case PAGE_L1:
                return SMALL_PAGE_SIZE;
                break;

                case PAGE_L2:
                return LARGE_PAGE_SIZE;
                break;

                default:
                panic("GetPageSize: unsupported page size found");
                break;
            }
        }

        prev_page_table = (u64 *)KERN_P2V(cur_entry.GetPhysAddr());
    }

    return 0;
}

/**
 * _free_table_recursive
 * A recursive method that iterates down into a virtual address through the page table levels.
 * This is only called for cur_pagetable values that are actually themselves page tables,
 * so every call must result in a FreePage at some point.
 *
 * Does **NOT** free the pages pointed to by the table- just the table itself.
 *
 * We perform the following check at each level:
 *   1. If we are at level 0, we must free this page- do so immediately and exit.
 *   2. Check every entry in the page table, if it is a non-leaf entry
 *      (aka another page table), recurse in.
 *   3. Free this page.
 */
void _free_table_recursive(u64 *cur_pagetable, usize level) {
    FRACTALPTE_CLASS cur_entry;

    if (PAGE_L1 == level) {
        FreePage((virt_t) cur_pagetable);
        return;
    }

    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        cur_entry.Clear();
        cur_entry.From(cur_pagetable[i]);

        // We are not at level 0, so calling IsLeaf() is defined
        if ((cur_entry.IsPresent() && !cur_entry.IsLeaf())) {
            virt_t *next_pagetable = (virt_t *)KERN_P2V(cur_entry.GetPhysAddr());
            _free_table_recursive(next_pagetable, level - 1);
        }
    }

    FreePage((virt_t) cur_pagetable);
}

u64 *AllocPageTable() {
    u64 *new_pt = (u64 *)AllocPage(SMALL_PAGE);

    // Clear main page table
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        new_pt[i] = PTE_UNMAPPED;
    }

    return new_pt;
}

/**
 * FreePageTable
 * Walks a given page table and frees every allocated page in it.
 */
void FreePageTable(u64 *page_table) {
    _free_table_recursive(page_table, N_PAGETABLE_LEVELS - 1);
}

/**
 * _free_everything_recursive
 * Like _free_table_recursive, except it frees every leaf physical page as well.
 *
 * _free_table_recursive just cleans up the page table, this method
 * cleans up the page table **AND** everything it points to, too, if
 * that physical page was allocated by the page allocator.
 *
 * Make sure the kernel is unmapped before calling this!
 */
void _free_everything_recursive(u64 *cur_pagetable, usize level) {
    FRACTALPTE_CLASS cur_entry;
    virt_t dest_page;

    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        cur_entry.Clear();
        cur_entry.From(cur_pagetable[i]);
        dest_page = KERN_P2V(cur_entry.GetPhysAddr());

        if (!cur_entry.IsPresent()) continue;

        if (PAGE_L1 == level || cur_entry.IsLeaf()) {
            // Skip over pages that don't point to allocated memory
            if (DUPLICATED_PAGE == cur_entry.GetExtraBits()) continue;

            if (IsAllocatedPage(dest_page)) {
                FreePage(dest_page);
            } else {
                panic("There's something pointed to by a page table that is not an allocated page, what??");
            }
        } else {
            // We are not at level 0 and not a leaf, so recurse
            _free_everything_recursive((u64 *)dest_page, level - 1);
        }
    }

    FreePage((virt_t) cur_pagetable);
}

/**
 * FreeUserPageTable
 * Walks a given page table and frees the page pointed to by
 * every userspace visible address. Then calls FreePageTable
 * to tear down the actual pages that make up the page table
 * itself.
 */
void FreeUserPageTable(u64 *page_table) {
    // First, make sure no kernel pages exist in the page table
    global_cpu.UnmapKernel(page_table);

    // This method recursively traverses the page table and frees
    // all physical pages pointed to by the page table, as well
    // as the page table itself.
    _free_everything_recursive(page_table, N_PAGETABLE_LEVELS - 1);
}

kret_t MapKernelContiguous(virt_t vaddr, usize n_largepages) {
    if (is_user_addr(vaddr)) {
        panic("Tried to make a temporary kernel mapping in a userspace vaddr");
    }

    u64 *kernel_pagetable = global_cpu.GetPageTable(vaddr);

    for (usize i = 0; i < n_largepages; i++) {
        virt_t new_pg = AllocPage(LARGE_PAGE);
        if (!new_pg) { panic("Failed to create temporary kernel mapping"); }

        PageMap(
            kernel_pagetable,
            vaddr + (LARGE_PAGE_SIZE * i),
            KERN_V2P(new_pg),
            LARGE_PAGE,
            KERNEL_PAGE
        );
    }

    global_cpu.FlushTLB();
    return ALL_GOOD;
}

kret_t UnmapKernelContiguous(virt_t vaddr, usize n_largepages) {
    if (is_user_addr(vaddr)) {
        panic("Tried to unmap a temporary kernel mapping in a userspace vaddr");
    }

    u64 *kernel_pagetable = global_cpu.GetPageTable(vaddr);

    for (usize i = 0; i < n_largepages; i++) {
        virt_t page_backing = PageWalk(
            kernel_pagetable,
            vaddr + (LARGE_PAGE_SIZE * i)
        );

        if (PHYS_ADDR_INVALID == page_backing) {
            panic("Tried to unmap non-present contiguous temporary kernel mapping");
        }

        FreePage(KERN_P2V(page_backing));

        PageUnmap(
            kernel_pagetable,
            vaddr + (LARGE_PAGE_SIZE * i)
        );
    }

    global_cpu.FlushTLB();
    return ALL_GOOD;
}

/*
 * _duplicate_page
 * Given a kernel-allocated page at `src', create a new
 * page on the kernel heap, copy src's contents into it,
 * and return the new page.
 *
 * Works for SMALL_PAGE and LARGE_PAGE sizes
 */
virt_t _duplicate_page(virt_t src, PageSize sz) {
    virt_t newpg = 0;
    if (!IsAllocatedPage(src)) panic("Tried to duplicate a page we didn't allocate- this isn't impossible, but it almost definitely isn't what we wanted to do and is probably a bug.");
    switch (sz) {
        case SMALL_PAGE:
        case LARGE_PAGE:
        newpg = AllocPage(sz);
        memcpy((u8*)newpg, (u8*)src, sizeof_page_flavor(sz));
        return newpg;
        break;

        default:
        panic("Tried to duplicate a page whose size is unsupported (%d)", sz);
        break;
    }
}

/*
 * _copy_pagetable_recursive
 * A recursive method that iterates down into a virtual address through the page table levels.
 *
 * When we reach a leaf in the src page table, we:
 *   1. Make a new page of the same size
 *   2. Copy everything from the original page into it
 *   3. Map it into the dst page table
 * 
 * At each level, for every mapped entry we check if it is a leaf or not.
 * If it is a leaf, we do the above.
 * Otherwise, recurse into cur_src_table (this holds the current 4K page we are looking at).
 *
 * We construct the virtual address as we traverse (because it relies on previous indices).
 * We store what we know of the VA thus far into va_so_far. Each level we can OR the bits we
 * learn with the contents of va_so_far to slowly build up the virtual address.
 *
 * Arguments:
 *  - dst_table: A pointer to the top 4K page of the destination page table
 *  - cur_src_table: The current 4K page table we are looking at from src, as we recurse this changes.
 *  - va_so_far: What we know of the virtual address thus far from every page we've seen.
 *  - level: How many levels deep are we? Starts at N_PAGETABLE_LEVELS - 1, goes to 0.
 *  - ignore_higher_half: Ignore pages in higher half of the address space from this page table.
 *                        Set this to true when calling this method to omit the kernel from the copy.
 *                        The kernel cannot be copied for many reasons, must be remapped in via global_cpu.MapKernel
 *                        (for one, the kernel uses giant pages, which we cannot copy, and also the kernel is huge, and
 *                        holds a copy of all of physical memory mapped, and also has lots of shared mutable state like
 *                        the kernel heap).
 */
void _copy_pagetable_recursive(u64 *dst_table, u64 *cur_src_table, virt_t va_so_far, usize level, bool ignore_higher_half) {
    FRACTALPTE_CLASS cur_entry;
    virt_t cur_entry_kern_physaddr;
    virt_t va_at_this_entry = 0;
    virt_t newly_copied_page = 0;

    usize num_pt_entries_at_this_level = NUM_PT_ENTRIES;
    if (ignore_higher_half) num_pt_entries_at_this_level /= 2;

    for (usize i = 0; i < num_pt_entries_at_this_level; i++) {
        va_at_this_entry = va_so_far | (i << (PAGE_OFFSET_BITS + (PAGE_BITS_PER_INDIRECTION_LEVEL * level)));
        cur_entry.Clear();
        cur_entry.From(cur_src_table[i]);
        cur_entry_kern_physaddr = KERN_P2V(cur_entry.GetPhysAddr());

        if (!cur_entry.IsPresent()) continue;

        if (PAGE_L1 == level) {
#ifndef QUIET_BOOT
            printf("Copying 0x%X->0x%X (small page) (%d:%d)\n", va_at_this_entry, cur_entry_kern_physaddr, level, i);
#endif // !QUIET_BOOT

            newly_copied_page = _duplicate_page(cur_entry_kern_physaddr, SMALL_PAGE);
            PageMap(dst_table, va_at_this_entry, KERN_V2P(newly_copied_page), SMALL_PAGE, cur_entry.GetPrivilege());
            continue;
        }

        if (PAGE_L2 == level && cur_entry.IsLeaf()) {
#ifndef QUIET_BOOT
            printf("Copying 0x%X->0x%X (large page)\n", va_at_this_entry, cur_entry_kern_physaddr);
#endif // !QUIET_BOOT
            newly_copied_page = _duplicate_page(cur_entry_kern_physaddr, LARGE_PAGE);
            PageMap(dst_table, va_at_this_entry, KERN_V2P(newly_copied_page), LARGE_PAGE, cur_entry.GetPrivilege());
            continue;
        }

        // If cur_entry is a leaf and we are at level 2, we found a giant page- can't copy those!
        if (PAGE_L3 == level && cur_entry.IsLeaf()) {
            panic("_copy_pagetable_recursive tried to copy a giant page- can't do that");
        }

        // printf("Recursing into page table at 0x%X (level %d)\n", cur_entry_kern_physaddr, level);
        _copy_pagetable_recursive((u64 *)dst_table, (u64 *)cur_entry_kern_physaddr, va_at_this_entry, level - 1, false);
    }
}

kret_t CopyPagetable(u64 *dst, u64 *src) {
    if (!dst || !src) return THING_DOESNT_EXIST;
    _copy_pagetable_recursive(dst, src, 0, N_PAGETABLE_LEVELS-1,true);
    return ALL_GOOD;
}

void _reconfigure_privs_recursive(u64 *table, usize level, bool ignore_higher_half, PagePrivilege p) {
    FRACTALPTE_CLASS cur_entry;
    virt_t next_pagetable;

    usize num_pt_entries_at_this_level = NUM_PT_ENTRIES;
    if (ignore_higher_half) num_pt_entries_at_this_level /= 2;

    for (usize i = 0; i < num_pt_entries_at_this_level; i++) {
        cur_entry.Clear();
        cur_entry.From(table[i]);
        next_pagetable = KERN_P2V(cur_entry.GetPhysAddr());

        if (!cur_entry.IsPresent()) continue;

        if ((PAGE_L1 == level) || (PAGE_L2 == level && cur_entry.IsLeaf())) {
            cur_entry.SetPrivilege(p);
            table[i] = cur_entry.Serialize();
            // return;
            continue;
        } else {
            // cur_entry.SetIntermediatePage(p);
            // table[i] = cur_entry.Serialize();
        }

        _reconfigure_privs_recursive((u64 *)next_pagetable, level - 1, false, p);
    }
}

/*
 * ReconfigurePagetablePrivileges
 * Recursively sets every page mapped in the user half of this page table
 * to either be a user page or kernel page (depending on what new_privs is).
 *
 * Ignores the upper half of the address space (kernel never gets remapped).
 */
kret_t ReconfigurePagetablePrivileges(u64 *pagetable, PagePrivilege new_privs) {
    _reconfigure_privs_recursive(pagetable, N_PAGETABLE_LEVELS - 1, true, new_privs);
    return ALL_GOOD;
}

/**
 * paging_test
 * Method that tests the paging methods in this file to make sure they work.
 */
extern "C" void paging_test() {
    printf("### PAGING TEST BEGIN ###\r\n");
    printf("-> Phase 1\r\n");
    u64 *top_pt = (u64 *)AllocPage(SMALL_PAGE);

    PageMap(top_pt, 0xA00000000000, 0x12340000, SMALL_PAGE, KERNEL_PAGE);
    PageMap(top_pt, 0xA00000001000, 0x56780000, SMALL_PAGE, KERNEL_PAGE);
    PageMap(top_pt, 0xA00000400000, 0x41410000, LARGE_PAGE, USER_PAGE);
    PageMap(top_pt, 0xA00400400000, 0x51510000, GIANT_PAGE, KERNEL_PAGE);

/*
EXPECTED PAGING STRUCTURE:

  LEVEL3        LEVEL2        LEVEL1        LEVEL0
+-------+     +-------+     +-------+     +-------+
|top_pt |-+-> |  idx  |-+-> |  idx  |-+>  |  idx  | -> 0x12340000
|       | |   |   0   | |   |   0   | |   |   0   |
+-------+ |   +-------+ |   +-------+ |   +-------+
          |             |             |
          |             |             |   +-------+
          |             |             +-> |  idx  | -> 0x56780000
          |             |                 |   1   |
          |             |                 +-------+
          |             |
          |             |   +-------+
          |             +-> |  idx  | -> 0x41410000
          |                 |   4   |
          |                 +-------+
          |
          |   +-------+
          +-> |  idx  | -> 0x51510000
              |   16  |
              +-------+
*/

    printf("Unmapping 0xA00000000000, shouldn't free anything:\r\n");
    PageUnmap(top_pt, 0xA00000000000);

/*
EXPECTED PAGING STRUCTURE:

  LEVEL3        LEVEL2        LEVEL1        LEVEL0
+-------+     +-------+     +-------+     +-------+
|top_pt |-+-> |  idx  |-+-> |  idx  |-+>  |  idx  | -> UNMAPPED
|       | |   |   0   | |   |   0   | |   |   0   |
+-------+ |   +-------+ |   +-------+ |   +-------+
          |             |             |
          |             |             |   +-------+
          |             |             +-> |  idx  | -> 0x56780000
          |             |                 |   1   |
          |             |                 +-------+
          |             |
          |             |   +-------+
          |             +-> |  idx  | -> 0x41410000
          |                 |   4   |
          |                 +-------+
          |
          |   +-------+
          +-> |  idx  | -> 0x51510000
              |   16  |
              +-------+
*/
    printf("Unmapping 0xA00000001000, should free exactly 1 small page:\r\n");
    PageUnmap(top_pt, 0xA00000001000);

/*
EXPECTED PAGING STRUCTURE:

  LEVEL3        LEVEL2        LEVEL1        LEVEL0
+-------+     +-------+     +-------+
|top_pt |-+-> |  idx  |-+-> |  idx  |->  UNMAPPED (This page was just freed)
|       | |   |   0   | |   |   0   |
+-------+ |   +-------+ |   +-------+
          |             |
          |             |   +-------+
          |             +-> |  idx  | -> 0x41410000
          |                 |   4   |
          |                 +-------+
          |
          |   +-------+
          +-> |  idx  | -> 0x51510000
              |   16  |
              +-------+
*/

    printf("Unmapping 0xA00000400000, should free 1 small page:\r\n");
    PageUnmap(top_pt, 0xA00000400000);

/*
EXPECTED PAGING STRUCTURE:

  LEVEL3        LEVEL2        LEVEL1
+-------+     +-------+
|top_pt |-+-> |  idx  |-> UNMAPPED (This page was just freed)
|       | |   |   0   |
+-------+ |   +-------+
          |
          |   +-------+
          +-> |  idx  | -> 0x51510000
              |   16  |
              +-------+
*/

    printf("Unmapping 0xA00400400000, should free 2 small pages:\r\n");
    PageUnmap(top_pt, 0xA00400400000);

/*
EXPECTED PAGING STRUCTURE:

  LEVEL3
+-------+
|top_pt |-> UNMAPPED (This page, as well as top_pt, were just freed)
|       |
+-------+
*/

    printf("-> Phase 2\r\n");
    top_pt = (u64 *)AllocPage(SMALL_PAGE);

    PageMap(top_pt, 0xA00000000000, 0x12340000, SMALL_PAGE, KERNEL_PAGE);
    PageMap(top_pt, 0xA00000001000, 0x56780000, SMALL_PAGE, KERNEL_PAGE);
    PageMap(top_pt, 0xA00000400000, 0x41410000, LARGE_PAGE, USER_PAGE);
    PageMap(top_pt, 0xA00400400000, 0x51510000, GIANT_PAGE, KERNEL_PAGE);

    printf("Freeing entire page table, we should see the same number of FreePage calls as we saw AllocPage calls after phase 2 began:\r\n");
    FreePageTable(top_pt);

    printf("### PAGING TEST COMPLETE ###\r\n");

    while(true);
}

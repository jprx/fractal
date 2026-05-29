#ifndef PAGING_H
#define PAGING_H

#include <types.h>
#include <lib/mem.h>
#include <page_alloc.h>
#include <virt_mem.h>
#include <out.h>
#include <page.h>
#include <task.h>
#include <syscall.h>
#include <memory.h>
#include <fmap.h>

BEGIN_C_HEADER

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
kret_t PageMap(u64 *page_table, virt_t virt, phys_t phys, PageSize sz, PagePrivilege priv);

/**
 * PageUnmap
 * Unmaps a virtual address from a page table, freeing all allocated pages as we go.
 * Undoes Map().
 *
 * Arguments:
 * - page_table: A pointer to the top level of a page table from which we will free the page
 * - virt: The virtual address to free
 *
 * Return Value:
 * If the entire page table has been freed, returns true. Otherwise, returns false.
 */
bool PageUnmap(u64 *page_table, virt_t virt);

/**
 * GigaMap
 * Creates a massive region of virtual addresses that map to the same phys page.
 * The phys page should be a large page from the page heap.
 * This method pretty much implicitly assumes 48 bit VAs.
 *
 * Arguments:
 *  - gmap: The gigamap_t for this page table
 *  - page_table: Page table to map into
 *  - virt: Giant page aligned VA to insert the gigamap
 *  - backing_va: Which VA do we map the large page that backs this gmap?
 *  - privs: What privilege should this gigamap be allocated with?
 */
kret_t GigaMap(gigamap_t *gmap, u64 *page_table, virt_t virt, virt_t backing_va, PagePrivilege privs);

/**
 * GigaMapCarveout
 * Carves out a single L2 page from this gigamap.
 *
 * Arguments:
 *  - gmap: The gigamap_t for this page table
 *  - page_table: Page table to map into
 *  - virt: The VA to carve out of the gigamap
 */
kret_t GigaMapCarveout(gigamap_t *gmap, u64 *page_table, virt_t virt);

/**
 * GigaMapGetBase
 * Allocates and returns a special page that always points to the gigamap's
 * base underlying physical address. No matter which pages are carved out
 * of the gigamap, this page will always point to the default PA.
 *
 * Arguments:
 *  - gmap: The gigamap_t for this page table
 *  - page_table: Page table to map into
 *  - outv: Where we write the VA on success
 */
kret_t GigaMapGetBase(gigamap_t *gmap, u64 *page_table, virt_t *outv);

/**
 * GigaMapReset
 * Removes and frees any carveouts from the gigamap inside this page table.
 *
 * Arguments:
 *  - gmap: The gigamap_t for this page table
 *  - page_table: Page table to map into
 */
kret_t GigaMapReset(gigamap_t *gmap, u64 *page_table);

/**
 * GigaMapUnmap
 * Removes a gigamap from the target page table, freeing all
 * structures allocated by GigaMap as well as any carveouts.
 *
 * Arguments:
 *  - gmap: The gigamap_t for this page table
 *  - page_table: Page table to map into
 */
kret_t GigaMapUnmap(gigamap_t *gmap, u64 *page_table);

/**
 * PageWalk
 * Walk a page table and return the physical address for this virtual one.
 * Returns PHYS_ADDR_INVALID if the address is not present.
 */
phys_t PageWalk(u64 *page_table, virt_t virt);

/**
 * IsPagePresent
 * Returns true if the virtual address virt is mapped in the page table page_table.
 */
bool IsPagePresent(u64 *page_table, virt_t virt);

/*
 * SetPageMemoryKind
 * Specify what kind of memory this page refers to.
 *
 * By default, whether pages mapped by PageMap are device memory or normal
 * memory is undefined by the paging API. In practice, I have configured
 * the registers (eg. MAIR on ARM64) such that all allocations are normal
 * memory unless specified otherwise.
 *
 * kind: What kind of memory is being mapped?
 */
void SetPageMemoryKind(u64 *page_table, virt_t virt, memory_kind_t kind);

/*
 * SetPageMemoryPermission
 * Reconfigure a virtual address's page privilege level (eg. user or kernel).
 */
void SetPageMemoryPermission(u64 *page_table, virt_t virt, PagePrivilege priv);

/*
 * Get/Set PageExtraBits
 * Read/ Write a constant into this page table entry's software-defined bits,
 * provided the mapping exists in the given page table.
 */
u8 GetPageExtraBits(u64 *page_table, virt_t virt);
void SetPageExtraBits(u64 *page_table, virt_t virt, u8 extra);

/*
 * GetPageSize
 * Returns how large of a page is mapped at virtual address `vaddr'.
 */
usize GetPageSize(u64 *page_table, virt_t vaddr);

/**
 * AllocPageTable
 * Returns a new SMALL_PAGE, filled with empty PTEs.
 */
u64 *AllocPageTable();

/**
 * FreePageTable
 * Walks a given page table and frees every allocated page in it.
 * This will free the entire page table, including the top-level page.
 */
void FreePageTable(u64 *page_table);

/**
 * FreeUserPageTable
 * Walks a given page table and frees the page pointed to by
 * every userspace visible address. Then calls FreePageTable
 * to tear down the actual pages that make up the page table
 * itself.
 */
void FreeUserPageTable(u64 *page_table);

/*
 * MapKernelContiguous
 * Make a temporary kernel mapping of contiguous virtual addresses.
 *
 * Arguments:
 *  - vaddr: The base at which to make this allocation
 *  - n_largepages: How many large pages should be used for it
 *
 * Kernel mappings are not preserved between interrupts, so everything
 * including this call to the unmap call must be part of a critical
 * section with interrupts disabled!
 *
 * The reason these are not persistent is because ARM uses a second
 * page table for the kernel which is static per-CPU, whereas X86
 * and RISCV use the task page table to find kernel pages.
 *
 * So if two tasks both tried to map contiguous memory to the same
 * location, X86 and RISCV users would see the "correct" pages,
 * whereas the early user of MapKernelContiguous on ARM would see
 * the pages mapped by the later caller.
 *
 * @TODO: When we add multicore support, make sure that each
 *        of these mappings are in a per-CPU area.
 */
kret_t MapKernelContiguous(virt_t vaddr, usize n_largepages);

/*
 * UnmapKernelContiguous
 * Unmap a temporary kernel mapping of contiguous virtual addresses,
 * created with MapKernelContiguous.
 *
 * Arguments:
 *  - vaddr: The base at which to make this allocation
 *  - n_largepages: How many large pages should be used for it
 *
 * The caller should be in the same critical section as they were
 * when they called MapKernelContiguous.
 */
kret_t UnmapKernelContiguous(virt_t vaddr, usize n_largepages);

/*
 * CopyPagetable
 * Copies everything from pagetable src into dst.
 * This will allocate new pages from the page heap and copy their
 * contents into new pages in dst.
 *
 * After this call, dst contains a full copy of the address space
 * of src, without any shared backing pages.
 *
 * Essentially an unoptimized full address space copy (no CoW- yet).
 */
kret_t CopyPagetable(u64 *dst, u64 *src);

/*
 * ReconfigurePagetablePrivileges
 * Recursively sets every page mapped in the user half of this page table
 * to either be a user page or kernel page (depending on what new_privs is).
 */
kret_t ReconfigurePagetablePrivileges(u64 *pagetbale, PagePrivilege new_privs);

END_C_HEADER

#endif // PAGING_H

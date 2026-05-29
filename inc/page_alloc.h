#ifndef PAGE_ALLOC_H
#define PAGE_ALLOC_H

#include <types.h>
#include <page.h>

BEGIN_C_HEADER

/*
 * Page
 * A struct that represents an allocated page.
 *
 * Whenever a page is allocated, a Page object
 * is created on the sbrk heap via dlmalloc().
 *
 * When the page should be released, a pointer
 * to the same Page object should be passed back.
 */
typedef struct page_allocation_t {
    // Virtual base address of the page:
    usize addr;
    PageSize size;
} Page;

/*
 * AllocPage()
 * Allocate a new page in the page allocator heap.
 * Returns NULL on failure.
 */
virt_t AllocPage(PageSize sz);

/*
 * FreePage
 * Frees a page previously allocated by AllocPage.
 */
void FreePage(virt_t);

/*
 * IsAllocatedPage
 * Returns true if the virtual address provided was
 * something that came from the page heap, false otherwise.
 */
bool IsAllocatedPage(virt_t);

/*
 * init_sbrk_heap
 * Tells frame allocator where to mark the page heap
 * used by page allocations.
 */
void init_page_heap(virt_t base, virt_t len);

END_C_HEADER

#endif // PAGE_ALLOC_H

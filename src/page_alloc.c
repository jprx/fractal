#include <page_alloc.h>
#include <lib/mem.h>
#include <fractal.h>
#include <lib/dlmalloc.h>
#include <out.h>
#include <page.h>
#include <regs.h>

// #define LOG_PAGE_ALLOC 1

// Set every freed page to a sentinel value to track UaFs
// #define PAGE_ALLOC_MEMSET_FREED

/*
 * STRATEGY
 * We want to allocate both large pages and small pages.
 * Small pages are allocated by first grabbing a large page
 * and then cutting it up into small pages.
 *
 * We allocate large pages for external use starting from
 * the top of the page heap, and small pages from the end
 * of the page heap going upwards.
 *
 * We start by dividing the page heap into a chunk for large
 * pages and a chunk for small pages. In the future we can
 * support flexible allocations but for now it is simpler to
 * just fix the number of large pages and small pages at init.
 *
 * beginning of page heap:
 * +--------------------+
 * |    Large Page 0    |
 * +--------------------+
 * |    Large Page 1    |
 * +--------------------+
 * |        ...         |
 * +--------------------+
 *           |
 *          \|/
 *  Growth of large page
 *      allocations
 *
 *
 *
 *  Growth of small page
 *      allocations
 *          /|\
 *           |
 * +--------------------+
 * |        ...         |
 * +--------------------+
 * |   Large Page N-1   | -> Divided into 1024 small pages
 * +--------------------+
 * |    Large Page N    | -> Divided into 1024 small pages
 * +--------------------+
 * end of page heap
 */

// How many of the large pages at the end are set aside for small pages:
#define MAX_LARGE_PAGES_FOR_SMALL_PAGES ((8))

// How many large pages available for allocation exist in the heap:
#define NUM_LARGE_PAGES ((((PAGE_HEAP_SIZE / LARGE_PAGE_SIZE)) - MAX_LARGE_PAGES_FOR_SMALL_PAGES))

#define NUM_SMALL_PAGES_PER_LARGE_PAGE ((LARGE_PAGE_SIZE / SMALL_PAGE_SIZE))

#define NUM_SMALL_PAGES ((MAX_LARGE_PAGES_FOR_SMALL_PAGES * NUM_SMALL_PAGES_PER_LARGE_PAGE))

// Base + length of the page heap
usize page_heap_base = 0;
usize page_heap_len = 0;

usize page_heap_large_page_start = 0;
usize page_heap_large_page_end = 0;
usize page_heap_small_page_start = 0;
usize page_heap_small_page_end = 0;

// Each bit here represents whether a large page is allocated or not
#define LARGE_PAGE_ALLOC_MASK_LEN ((NUM_LARGE_PAGES / BITS_PER_U8))
#define SMALL_PAGE_ALLOC_MASK_LEN (((MAX_LARGE_PAGES_FOR_SMALL_PAGES * NUM_SMALL_PAGES_PER_LARGE_PAGE) / BITS_PER_U8))

// Given an index, right shift it by this amount before indexing into the large page alloc mask
#define INDEX_TO_PAGE_ALLOC_SHIFT ((3))

// Given an index, & it with this mask to find the bit within a given field
#define INDEX_TO_PAGE_ALLOC_MASK ((BITS_PER_U8 - 1))

u8 large_page_alloc_mask[LARGE_PAGE_ALLOC_MASK_LEN] = {0};
u8 small_page_alloc_mask[SMALL_PAGE_ALLOC_MASK_LEN] = {0};

bool is_page_alloced_at_index(usize idx, u8 *bitarray) {
    return BIT_AT(idx & INDEX_TO_PAGE_ALLOC_MASK,
        bitarray[idx>>INDEX_TO_PAGE_ALLOC_SHIFT]
    );
}

void set_page_alloc_bit(usize idx, bool allocated, u8 *bitarray) {
    if (allocated) {
        bitarray[idx >> INDEX_TO_PAGE_ALLOC_SHIFT] |= BIT(idx & INDEX_TO_PAGE_ALLOC_MASK);
    } else {
        bitarray[idx >> INDEX_TO_PAGE_ALLOC_SHIFT] &= ~BIT(idx & INDEX_TO_PAGE_ALLOC_MASK);
    }
}

/*
 * _alloc_page
 * Internal method for allocating a page of arbitrary size
 * bitarray: The bit array that controls access to this heap
 *           (either large_page_alloc_mask or small_page_alloc_mask).
 * num_possible_pages: How many pages are allowed of this size?
 *                     (either NUM_LARGE_PAGES or NUM_SMALL_PAGES)
 */
usize _alloc_page(u8 *bitarray, usize num_possible_pages) {
    for (usize i = 0; i < num_possible_pages; i++) {
        if (!is_page_alloced_at_index(i, bitarray)) {
            set_page_alloc_bit(i, true, bitarray);
            return i;
        }
    }

    panic("Heap Exhausted- Out of memory trying to allocate page from bitarray 0x%X", bitarray);
    return -1;
}

usize _alloc_large_page() {
    usize idx = _alloc_page(large_page_alloc_mask, NUM_LARGE_PAGES);
    return (idx * LARGE_PAGE_SIZE) + page_heap_large_page_start;
}

usize _alloc_small_page() {
    usize idx = _alloc_page(small_page_alloc_mask, NUM_SMALL_PAGES);
    return (idx * SMALL_PAGE_SIZE) + page_heap_small_page_start;
}

/*
 * _free_page
 * Internal method for freeing a page of arbitrary size
 * p: The virtual address of the page to free
 * page_size: The size of the page being freed (either LARGE_PAGE or SMALL_PAGE)
 * heap_start: Where in the heap virtual memory is the start of the heap for this page size
 *             (either page_heap_large_page_start or page_heap_small_page_start).
 * bitarray: The bit array that controls access to this heap
 *           (either large_page_alloc_mask or small_page_alloc_mask).
 */
void _free_page(usize p, usize page_size, usize heap_start, u8 *bitarray) {
    usize p_index = (p - heap_start) / page_size;
    if ((p & (page_size - 1)) != 0) {
        panic("Freeing misaligned page: 0x%X", p);
    }
    if (!is_page_alloced_at_index(p_index, bitarray)) {
        panic("Double-free of page: 0x%X", p);
    }

    if (addr_in_page((u64)get_current_sp(), p, page_size)) {
        panic("tried to free the current kernel stack page");
    }

    set_page_alloc_bit(p_index, false, bitarray);
}

void _free_large_page(usize p) {
    _free_page(p,
        LARGE_PAGE_SIZE,
        page_heap_large_page_start,
        large_page_alloc_mask
    );
}

void _free_small_page(usize p) {
    _free_page(p,
        SMALL_PAGE_SIZE,
        page_heap_small_page_start,
        small_page_alloc_mask
    );
}

/*
 * AllocPage()
 * Allocate a new page in the page allocator heap.
 */
virt_t AllocPage(PageSize sz) {
    if (LARGE_PAGE == sz) {
        usize new_page = _alloc_large_page();
        if (0 == new_page) {
            panic("Heap exhausted: no more large pages");
        }
#ifdef LOG_PAGE_ALLOC
        printf("AllocPage(large): 0x%X\r\n", new_page);
#endif // LOG_PAGE_ALLOC
        return new_page;
    }

    if (SMALL_PAGE == sz) {
        usize new_page = _alloc_small_page();
        if (0 == new_page) {
            panic("Heap exhausted: no more small pages");
        }
#ifdef LOG_PAGE_ALLOC
        printf("AllocPage(small): 0x%X\r\n", new_page);
#endif // LOG_PAGE_ALLOC
        return new_page;
    }

    panic("Invalid page request");
    return 0;
}

/*
 * FreePage
 * Frees a page previously allocated by AllocPage.
 */
void FreePage(usize p) {
    if (0 == p) panic("Freeing null large page");

    if (p < page_heap_base || p > page_heap_base + page_heap_len) {
        panic("Freeing out of bounds page 0x%X", p);
    }

    if (p < page_heap_large_page_end) {
#ifdef LOG_PAGE_ALLOC
        printf("FreePage(large):   0x%X\r\n", p);
#endif // LOG_PAGE_ALLOC
        _free_large_page(p);
#ifdef PAGE_ALLOC_MEMSET_FREED
        // printf("Filling freed large page\r\n");
        memset((u8 *)p, '\x41', LARGE_PAGE_SIZE);
#endif // PAGE_ALLOC_MEMSET_FREED
    } else {
#ifdef LOG_PAGE_ALLOC
        printf("FreePage(small):   0x%X\r\n", p);
#endif // LOG_PAGE_ALLOC
        _free_small_page(p);
#ifdef PAGE_ALLOC_MEMSET_FREED
        // printf("Filling freed small page\r\n");
        memset((u8 *)p, '\x41', SMALL_PAGE_SIZE);
#endif // PAGE_ALLOC_MEMSET_FREED
    }
}

bool IsAllocatedPage(virt_t p) {
    return !(p < page_heap_base || p > page_heap_base + page_heap_len);
}

void init_page_heap(usize base, usize len) {
    page_heap_base = base;
    page_heap_len = len;
    memset(large_page_alloc_mask, '\x00', LARGE_PAGE_ALLOC_MASK_LEN);
    memset(small_page_alloc_mask, '\x00', SMALL_PAGE_ALLOC_MASK_LEN);

    page_heap_large_page_start = page_heap_base;
    page_heap_large_page_end = page_heap_base + (NUM_LARGE_PAGES * LARGE_PAGE_SIZE);

    // Set aside MAX_LARGE_PAGES_FOR_SMALL_PAGES large pages for small pages
    page_heap_small_page_start = page_heap_large_page_end;
    page_heap_small_page_end = page_heap_small_page_start + (MAX_LARGE_PAGES_FOR_SMALL_PAGES * LARGE_PAGE_SIZE);
}

void dump_page_allocator_info() {
    printf("== Page Allocator ==\r\n");
    printf("       page allocator: [0x%X-0x%X]\r\n", page_heap_base, page_heap_base + page_heap_len);
    printf("    large page region: [0x%X-0x%X]\r\n", page_heap_large_page_start, page_heap_large_page_end);
    printf("    small page region: [0x%X-0x%X]\r\n", page_heap_small_page_start, page_heap_small_page_end);
    printf("      NUM_LARGE_PAGES: %d\r\n", NUM_LARGE_PAGES);
    printf("      NUM_SMALL_PAGES: %d\r\n", NUM_SMALL_PAGES);
}

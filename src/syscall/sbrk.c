#include <fractal.h>
#include <syscall.h>
#include <paging.h>
#include <task.h>

i64 sys_sbrk(i32 incr) { return do_sys_sbrk(current_task(), incr); }

i64 do_sys_sbrk(task_t *t, i32 incr) {
#ifdef DEBUG_SBRK
    printf("[sbrk] incr by 0x%X\n", incr);
#endif // DEBUG_SBRK

    if (incr < 0) return t->t_heap_top;

    if ((virt_t)NULL == t->t_heap_top) {
        t->t_heap_top = DEFAULT_USER_HEAP;
    }

    virt_t prev_end = t->t_heap_top;

    // For now, scan the whole heap and alloc pages that are missing
    // Easier and less error-prone than trying to calculate how many pages we need to add onto the end (and dealing with off by 1 conditions)
    for (usize i = DEFAULT_USER_HEAP; i < ALIGN_LARGE_PAGE(t->t_heap_top + incr) + LARGE_PAGE_SIZE; i += LARGE_PAGE_SIZE) {
#ifdef DEBUG_SBRK
        printf("[sbrk] checking 0x%X\n", i);
#endif // DEBUG_SBRK
        if (!IsPagePresent(t->t_pagetable, i)) {
#ifdef DEBUG_SBRK
            printf("[sbrk] allocating new page\n", i);
#endif // DEBUG_SBRK
            virt_t new_page_virt = AllocPage(LARGE_PAGE);
            phys_t new_page_phys = KERN_V2P(new_page_virt);

            if ((virt_t)0 == new_page_virt) panic("sbrk failed to alloc a new page");

            PageMap(
                t->t_pagetable,
                i,
                new_page_phys,
                LARGE_PAGE,
                USER_PAGE
            );
        }
    }

    t->t_heap_top += incr;

    if (!IsPagePresent(t->t_pagetable, t->t_heap_top)) {
        panic("sbrk failed: we have an off by 1 error somewhere");
    }

    return prev_end;
}

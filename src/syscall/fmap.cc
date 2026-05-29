#include <fractal.h>
#include <fmap.h>
#include <syscall.h>
#include <paging.h>
#include <task.h>
#include <copy_task.h>
#include <arch.h>
#include <virt_mem.h>
#include <lib/dlmalloc.h>

void insert_fmap_alloc_to_task(task_t *t, fmap_alloc_t *a) {
    a->f_next = t->t_fmaplist;
    t->t_fmaplist = a;
}

bool check_overlap(fmap_alloc_t *a, fmap_alloc_t *b) {
    for (usize a_pg = 0; a_pg < a->f_n_copies; a_pg++) {
        for (usize b_pg = 0; b_pg < b->f_n_copies; b_pg++) {
            usize a_offset = (a_pg * (1ull << a->f_log_stride) * LARGE_PAGE_SIZE);
            usize b_offset = (b_pg * (1ull << b->f_log_stride) * LARGE_PAGE_SIZE);
            if (a->f_base_va + a_offset == b->f_base_va + b_offset) return true;
        }
    }

    return false;
}

bool overlapping_alloc_exists_in_task(task_t *t, fmap_alloc_t *a) {
    fmap_alloc_t *cursor = t->t_fmaplist;
    while (cursor) {
        if (check_overlap(a, cursor)) return true;
        cursor = cursor->f_next;
    }
    return false;
}

fmap_alloc_t *search_task_for_alloc(task_t *t, usize base_va) {
    fmap_alloc_t *cursor = t->t_fmaplist;
    while (cursor) {
        if (base_va == cursor->f_base_va) {
            return cursor;
        }
        cursor = cursor->f_next;
    }

    return NULL;
}

bool delete_alloc_if_exists_in_task(task_t *t, fmap_alloc_t *a) {
    fmap_alloc_t *cursor = t->t_fmaplist;
    fmap_alloc_t **prev_ptr = &t->t_fmaplist;
    while (cursor) {
        if (cursor == a) {
            *prev_ptr = cursor->f_next;
            dlfree(cursor);
            return true;
        }
        prev_ptr = &cursor->f_next;
        cursor = cursor->f_next;
    }

    return false;
}

void cleanup_fmap_list(task_t *t) {
    fmap_alloc_t *cursor = t->t_fmaplist;
    fmap_alloc_t *prev = NULL;
    while (cursor) {
        prev = cursor;
        cursor = cursor->f_next;
        printf("[fmap] cleaning up 0x%X\n", prev);
        dlfree(prev);
    }
}

i64 do_sys_fmap(task_t *t, virt_t u_newpgptr, usize n_copies, usize log_stride) {
    u64 target_va = t->t_fmap_top;

    if (0 == n_copies) return -1;

    if (target_va < DEFAULT_USER_PAGE_HEAP) panic("task fmap heap top is too small (0x%X)", t->t_fmap_top);

    usize highest_va = target_va + LARGE_PAGE_SIZE + ((n_copies-1) * LARGE_PAGE_SIZE * (1ull << log_stride));
    if (!is_user_addr(target_va) || !is_user_addr(highest_va)) return -2;

    virt_t new_page_virt = AllocPage(LARGE_PAGE);
    phys_t new_page_phys = KERN_V2P(new_page_virt);
    if ((virt_t)0 == new_page_virt) panic("fmap failed to alloc a new page");

    usize prev_top = t->t_fmap_top;
    t->t_fmap_top += (highest_va - target_va);
    if (t->t_fmap_top - prev_top < LARGE_PAGE_SIZE) panic("didn't increment the heap top sufficiently");

    fmap_alloc_t *new_alloc = (fmap_alloc_t*)dlmalloc(sizeof(*new_alloc));
    if (!new_alloc) panic("failed to allocate new fmap allocation request object");
    memset(new_alloc, '\x00', sizeof(*new_alloc));
    new_alloc->f_next = NULL;
    new_alloc->f_base_va = target_va;
    new_alloc->f_n_copies = n_copies;
    new_alloc->f_log_stride = log_stride;

    // overlaps shouldn't be possible because we shift the heap top by the size of the alloc each time
    // yet it's still probably a good idea to confirm no overlaps in case that ever changes.
    if (overlapping_alloc_exists_in_task(t, new_alloc)) {
        printf("[fmap] allocation overlaps with an existing allocation\n");
        dlfree(new_alloc);
        FreePage(new_page_virt);
        return -3;
    }

    insert_fmap_alloc_to_task(t, new_alloc);

    for (usize i = 0; i < n_copies; i++) {
        u64 va_to_set = target_va + (LARGE_PAGE_SIZE * i * (1ull << log_stride));

        if (PHYS_ADDR_INVALID != PageWalk(t->t_pagetable, va_to_set)) {
            panic("do_sys_fmap: tried to allocate at 0x%X but something is already there", va_to_set);
        }

        PageMap(
            t->t_pagetable,
            va_to_set,
            new_page_phys,
            LARGE_PAGE,
            KERNEL_PAGE
        );

        SetPageExtraBits(
            t->t_pagetable,
            va_to_set,
            i == 0 ? ALLOCATED_PAGE : DUPLICATED_PAGE
        );
    }

    global_cpu.FlushTLB();
    copy_to_task(t, u_newpgptr, &target_va, sizeof(target_va));
    return 0;
}

i64 sys_fmap(virt_t u_newpgptr, usize n_copies, usize log_stride) {
    return do_sys_fmap(current_task(), u_newpgptr, n_copies, log_stride);
}

i64 do_sys_funmap(task_t *t, virt_t u_pgptr) {
    if (!IsPagePresent(t->t_pagetable, u_pgptr)) return -1;
    if (!is_user_addr(u_pgptr)) return -2;
    if (u_pgptr < DEFAULT_USER_PAGE_HEAP) return -3;

    virt_t target_va = u_pgptr;
    phys_t pa_to_free = PageWalk(t->t_pagetable, target_va);
    fmap_alloc_t *alloc_req = search_task_for_alloc(t, u_pgptr);
    if (!alloc_req) {
        printf("[funmap] couldn't find request in our list (0x%X)\n", u_pgptr);
        return -1;
    }

    for (usize i = 0; i < alloc_req->f_n_copies; i++) {
        u64 va_to_free = target_va + (LARGE_PAGE_SIZE * i * (1ull << alloc_req->f_log_stride));

        if (PageWalk(t->t_pagetable, va_to_free) != pa_to_free) panic("invalid phys page in fmap alloc");

        PageUnmap(
            t->t_pagetable,
            va_to_free
        );
    }

    FreePage(KERN_P2V(pa_to_free));

    if (!delete_alloc_if_exists_in_task(t, alloc_req)) {
        panic("failed to remove fmap allocation request");
    }

    global_cpu.FlushTLB();
    return 0;
}

i64 sys_funmap(virt_t u_pgptr) {
    return do_sys_funmap(current_task(), u_pgptr);
}


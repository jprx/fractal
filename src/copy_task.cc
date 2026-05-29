#include <fractal.h>
#include <copy_task.h>
#include <task.h>
#include <paging.h>

extern "C" kret_t copy_from_task(task_t *t, void *dst, virt_t user_buf, usize sz) {
    usize page_size, offset_into_page, bytes_to_copy;
    usize bytes_remaining = sz;
    usize bytes_read_so_far = 0;
    virt_t user_cursor = user_buf;

    if (!t) return THING_DOESNT_EXIST;
    u64 *page_table = task_get_pagetable(t);
    if (!page_table) return THING_DOESNT_EXIST;

    if (IS_KERNEL_VA(user_buf)) {
        panic("Tried to copy_from_user from a kernel address (0x%X)", user_buf);
    }

    if (!IsPagePresent(page_table, user_cursor)) return THING_DOESNT_EXIST;

    // Only (maybe) nonzero offset for first page
    // all other pages are page-aligned (offset is zero)
    page_size = GetPageSize(page_table, user_cursor);
    if (0 == page_size) {
        // 0 means this page is not present or we don't understand its page size.
        // Rather than divide by zero, just panic here.
        panic("Zero page size for 0x%X", user_cursor);
    }
    offset_into_page = user_cursor % page_size;

    while (bytes_remaining > 0) {
        if (IsPagePresent(page_table, user_cursor)) {
            bytes_to_copy = min(bytes_remaining, page_size - offset_into_page);
            memcpy(
                &((u8 *)dst)[bytes_read_so_far],
                (u8 *)KERN_P2V(PageWalk(page_table, user_cursor)),
                bytes_to_copy
            );
        } else {
            page_size = SMALL_PAGE_SIZE;
            bytes_to_copy = min(bytes_remaining, page_size - offset_into_page);
            memset(
                &((u8 *)dst)[bytes_read_so_far],
                '\x00',
                bytes_to_copy
            );
        }

        user_cursor += bytes_to_copy;
        bytes_read_so_far += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;

        page_size = GetPageSize(page_table, user_cursor);
        offset_into_page = 0;
    }

    return ALL_GOOD;
}

extern "C" kret_t copy_to_task(task_t *t, virt_t user_buf, void *buf, usize sz) {
    usize page_size, offset_into_page, bytes_to_copy;
    usize bytes_remaining = sz;
    usize bytes_read_so_far = 0;
    virt_t user_cursor = user_buf;

    if (!t) return THING_DOESNT_EXIST;
    u64 *page_table = task_get_pagetable(t);
    if (!page_table) return THING_DOESNT_EXIST;

    if (IS_KERNEL_VA(user_buf)) {
        panic("Tried to copy_to_task to a kernel address (0x%X)", user_buf);
    }

    if (!IsPagePresent(page_table, user_cursor)) return THING_DOESNT_EXIST;

    // Only (maybe) nonzero offset for first page
    // all other pages are page-aligned (offset is zero)
    page_size = GetPageSize(page_table, user_cursor);
    if (0 == page_size) {
        panic("Zero page size for 0x%X", user_cursor);
    }
    offset_into_page = user_cursor % page_size;

    while (bytes_remaining > 0) {
        bytes_to_copy = min(bytes_remaining, page_size - offset_into_page);

        if (IsPagePresent(page_table, user_cursor)) {
            memcpy(
                (u8 *)KERN_P2V(PageWalk(page_table, user_cursor)),
                &((u8 *)buf)[bytes_read_so_far],
                bytes_to_copy
            );
        }

        user_cursor += bytes_to_copy;
        bytes_read_so_far += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;

        page_size = GetPageSize(page_table, user_cursor);
        offset_into_page = 0;
    }

    return ALL_GOOD;
}

extern "C" kret_t copy_file_to_task(task_t *t, virt_t user_buf, void *file_handle, usize offset, usize sz) {
    usize page_size, offset_into_page, bytes_to_copy;
    usize bytes_remaining = sz;
    usize bytes_read_so_far = 0;
    virt_t user_cursor = user_buf;

    if (!t) return THING_DOESNT_EXIST;
    u64 *page_table = task_get_pagetable(t);
    if (!page_table) return THING_DOESNT_EXIST;

    if (IS_KERNEL_VA(user_buf)) {
        panic("Tried to copy_file_to_task to a kernel address (0x%X)", user_buf);
    }

    // Only (maybe) nonzero offset for first page
    // all other pages are page-aligned (offset is zero)
    page_size = GetPageSize(page_table, user_cursor);
    offset_into_page = user_cursor % page_size;

    while (bytes_remaining > 0) {
        bytes_to_copy = min(bytes_remaining, page_size - offset_into_page);

        if (IsPagePresent(page_table, user_cursor)) {
            filesys_read(
                file_handle,
                (u8 *)KERN_P2V(PageWalk(page_table, user_cursor)),
                bytes_to_copy,
                offset + bytes_read_so_far
            );
        }

        user_cursor += bytes_to_copy;
        bytes_read_so_far += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;

        page_size = GetPageSize(page_table, user_cursor);
        offset_into_page = 0;
    }

    return ALL_GOOD;
}

kret_t memset_to_task(task_t *t, virt_t user_dest, i32 val, usize sz) {
    usize page_size, offset_into_page, bytes_to_copy;
    usize bytes_remaining = sz;
    usize bytes_read_so_far = 0;
    virt_t user_cursor = user_dest;

    if (!t) return THING_DOESNT_EXIST;
    u64 *page_table = task_get_pagetable(t);
    if (!page_table) return THING_DOESNT_EXIST;

    // Only (maybe) nonzero offset for first page
    // all other pages are page-aligned (offset is zero)
    page_size = GetPageSize(page_table, user_cursor);
    offset_into_page = user_cursor % page_size;

    while (bytes_remaining > 0) {
        bytes_to_copy = min(bytes_remaining, page_size - offset_into_page);

        if (0 == bytes_to_copy) return ALL_GOOD;

        if (IsPagePresent(page_table, user_cursor)) {
            memset(
                (u8 *)KERN_P2V(PageWalk(page_table, user_cursor)),
                val,
                bytes_to_copy
            );
        } else {
            return ALL_GOOD;
        }

        user_cursor += bytes_to_copy;
        bytes_read_so_far += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;

        page_size = GetPageSize(page_table, user_cursor);
        offset_into_page = 0;
    }

    return ALL_GOOD;
}

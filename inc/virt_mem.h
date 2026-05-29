#ifndef VIRT_MEM_H
#define VIRT_MEM_H

// Cross-architecture canonical higher half virtual address mask:
#define KERNEL_VIRT_MASK ((0xFFFF800000000000))

// Physical memory is mapped as device memory at this virtual address:
#define KERNEL_DEVMEM_VIRT_MASK ((0xFFFFD00000000000))
#define KERNEL_DEVICEMAP_BEGIN ((KERNEL_DEVMEM_VIRT_MASK))

// Physical memory mapped as video memory (write-combining):
#define KERNEL_WRITECOMB_VIRT_MASK ((0xFFFFF00000000000))
#define KERNEL_WRITECOMB_BEGIN ((KERNEL_WRITECOMB_VIRT_MASK))

#define LOWER_HALF_END_VADDR     ((0x0000800000000000ull))
#define HIGHER_HALF_BEGIN_VADDR  ((0xFFFF800000000000ull))

// Anything less than this is user, anything more is kernel:
#define HALFWAY_VIRT_ADDR ((0x8000000000000000ull))

#define KERN_V2P(a) ((((a)) & ~KERNEL_VIRT_MASK))
#define KERN_P2V(a) ((((a)) | KERNEL_VIRT_MASK))

// Physical to device virtual and vice versa
#define KERN_P2DV(a) ((((a)) | KERNEL_DEVMEM_VIRT_MASK))
#define KERN_DV2P(a) ((((a)) & ~KERNEL_DEVMEM_VIRT_MASK))

// Physical to writecomb virtual and vice versa
#define KERN_P2WCV(a) ((((a)) | KERNEL_WRITECOMB_VIRT_MASK))
#define KERN_WCV2P(a) ((((a)) & ~KERNEL_WRITECOMB_VIRT_MASK))

#define ALIGN_16(a) ((((a & (~(0x10ull - 1ull))))))
#define ALIGN_SMALL_PAGE(a) ((((a & (~(SMALL_PAGE_SIZE - 1))))))
#define ALIGN_LARGE_PAGE(a) ((((a & (~(LARGE_PAGE_SIZE - 1))))))
#define IS_ALIGNED_SMALL_PAGE(a) (((0 == (a & (SMALL_PAGE_SIZE - 1)))))
#define IS_ALIGNED_LARGE_PAGE(a) (((0 == (a & (SMALL_PAGE_SIZE - 1)))))
#define OFFSET_IN_SMALL_PAGE(a) ((((a & (SMALL_PAGE_SIZE - 1)))))
#define OFFSET_IN_LARGE_PAGE(a) ((((a & (LARGE_PAGE_SIZE - 1)))))

// All kernel virtual addresses live in the higher-half region
#define IS_KERNEL_VA(v) (( v & ((1ull << 48ull)) ))
#define IS_USER_VA(v)   (( v < ((1ull << 48ull)) ))

#define DEFAULT_USER_PC              0x000004000000ull
#define DEFAULT_USER_HEAP            0x555555000000ull
#define DEFAULT_USER_PAGE_HEAP       0x707070000000ull
#define DEFAULT_USER_STACK_PAGE      0x7FFFF0000000ull
#define PRIMARY_GMAP_START           0x008000000000ull
#define TO_SECONDARY_GMAP            0x400000000000ull
#define PRIMARY_GMAP_BACKING_VA      0x004000000000ull
#define SECONDARY_GMAP_BACKING_VA    0x005000000000ull

#define STACK_SHIFT_BY_TASK_ID(t) (((t << 24ull)))
#define PAGE_TO_STACK_BOTTOM_OFFSET ((LARGE_PAGE_SIZE - 16))

/*
 * GET_STACK_BASE(p)
 * Given a large page p, returns the base of that page
 * as if the whole thing was a stack.
 */
#define GET_STACK_BASE(p) ((((p + PAGE_TO_STACK_BOTTOM_OFFSET))))

static inline bool addr_in_page(virt_t addr, virt_t page, usize pagelen) {
    return (addr >= page) && (addr < (page + pagelen));
}

static inline bool is_user_addr(virt_t va) {
    return ((va < HALFWAY_VIRT_ADDR));
}

static inline bool is_canonical(virt_t va) {
    return (va < LOWER_HALF_END_VADDR || va > HIGHER_HALF_BEGIN_VADDR);
}

#endif // VIRT_MEM_H

#ifndef TYPES_H
#define TYPES_H

// Xcompiler builds lib/gcc/$(TOOLCHAIN)/14.1.0/include/stdint.h
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef u64 usize;
typedef i64 isize;

typedef usize virt_t;
typedef usize phys_t;

#define BIT(n) (1ULL << ((n)))
#define MASK(x) (BIT(x) - 1)
#define BIT_AT(n, val) (((((val)) & BIT(((n)))) >> ((n))))
#define BITS_PER_U8 8

/* Various error handling types */
// kret_t is the main kernel return type,
// returning one of the various error values below
typedef enum kret_enum_t {
    ALL_GOOD = 0,
    OH_NO = -1,
    EINVAL = -2,
    ENOMEM = -3,
    THING_DOESNT_EXIST = -4,
    IDK_WHAT_THAT_THING_IS = -5,
    TIMEOUT = -6,
    OUT_OF_BOUNDS = -7,
} kret_t;

/*
 * PageSize
 * Tracks the various possible page sizes on the system
 */
typedef enum page_size_t {
    // these pages are allocatable to physical memory:
    SMALL_PAGE,
    LARGE_PAGE,

    // these pages are purely used for virtual mappings
    // (eg. mapping in the kernel and gigamaps)
    GIANT_PAGE,
    SUPER_PAGE,
    UNMAPPED_PAGE = -1,
} PageSize;

typedef enum {
    KERNEL_PAGE,
    USER_PAGE,
} PagePrivilege;

typedef enum {
    ALLOCATED_PAGE  = 0, // [default] This PTE points to a page that needs to be freed when the page table is freed.
    DUPLICATED_PAGE = 1, // This PTE points to a page that exists elsewhere in this page table, so no need to free it when cleaning up.
} PageBackingMemoryKind;

typedef enum {
    PAGE_L1 = 0,
    PAGE_L2 = 1,
    PAGE_L3 = 2,
    PAGE_L4 = 3,

    N_PAGETABLE_LEVELS,
} pagelevel_t;

#define PAGE_BITS_PER_INDIRECTION_LEVEL 9ull
#define PAGE_OFFSET_BITS 12ull

#define ONE_KB ((BIT(10)))
#define ONE_MB ((BIT(20)))
#define ONE_GB ((BIT(30)))

#define PAGE_SIZE_AT_LEVEL(l) (( BIT( (PAGE_OFFSET_BITS) + (l * (PAGE_BITS_PER_INDIRECTION_LEVEL) ) ) ))

#define SMALL_PAGE_SIZE ((PAGE_SIZE_AT_LEVEL(0)))
#define LARGE_PAGE_SIZE ((PAGE_SIZE_AT_LEVEL(1)))
#define GIANT_PAGE_SIZE ((PAGE_SIZE_AT_LEVEL(2)))
#define SUPER_PAGE_SIZE ((PAGE_SIZE_AT_LEVEL(3)))

#define NUM_PT_ENTRIES ((SMALL_PAGE_SIZE / sizeof(u64)))

#define PHYS_ADDR_INVALID ((0xAAAAAAAAAAAAAAAA))

static inline usize sizeof_page_flavor(PageSize in) {
    switch(in) {
        case SMALL_PAGE: return SMALL_PAGE_SIZE; break;
        case LARGE_PAGE: return LARGE_PAGE_SIZE; break;
        case GIANT_PAGE: return GIANT_PAGE_SIZE; break;
        default: return 0; break;
    }
}

/* Wrap all function declarations in headers that
 * point to C functions with this */
#ifdef __cplusplus
#define BEGIN_C_HEADER extern "C" {
#define END_C_HEADER }
#else // __cplusplus
#define BEGIN_C_HEADER
#define END_C_HEADER
#endif // !__cplusplus

#endif // TYPES_H

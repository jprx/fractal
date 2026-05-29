#include <types.h>
#include <fractal.h>
#include <lib/mem.h>
#include <out.h>
#include <kasan.h>

static u8 *heap_ptr = NULL;
usize sbrk_heap_base = 0;
usize sbrk_heap_len = 0;

bool __fast_memops_active = false;

#if !defined(KASAN) && defined(ARCH_HAS_FAST_MEMCPY)

void *_memcpy_slow(void *restrict dst, const void *restrict src, size_t n) {
    for (usize i = 0; i < n; i++) {
        ((u8 *)dst)[i] = ((u8 *)src)[i];
    }
    return dst;
}

void *_memcpy_fast(void *restrict dst, const void *restrict src, size_t n);
void *memcpy(void *restrict dst, const void *restrict src, size_t n) {
    if (__fast_memops_active) return _memcpy_fast(dst,src,n);
    else return _memcpy_slow(dst,src,n);
}

#else // !defined(KASAN) && defined(ARCH_HAS_FAST_MEMCPY)

void *memcpy(void *restrict dst, const void *restrict src, size_t n) {
    for (usize i = 0; i < n; i++) {
        ((u8 *)dst)[i] = ((u8 *)src)[i];
    }
    return dst;
}

#endif

#if !defined(KASAN) && defined(ARCH_HAS_FAST_MEMSET)

void *_memset_slow(void *b, int c, size_t len) {
    u8 *buf = (u8 *)b;
    for (usize i = 0; i < len; i++) {
        buf[i] = (u8)c;
    }
    return b;
}

void *_memset_fast(void *b, int c, size_t len);
void *memset(void *b, int c, size_t len) {
    if (__fast_memops_active) return _memset_fast(b,c,len);
    else return _memset_slow(b,c,len);
}

#else // !defined(KASAN) && defined(ARCH_HAS_FAST_MEMSET)

void *memset(void *b, int c, size_t len) {
    u8 *buf = (u8 *)b;
    for (usize i = 0; i < len; i++) {
        buf[i] = (u8)c;
    }
    return b;
}

#endif // ARCH_HAS_FAST_MEMSET

bool memcmp(const void *a, const void *b, usize s) {
    for (usize i = 0; i < s; i++) {
        if (((const u8 *)a)[i] != ((const u8 *)b)[i]) return false;
    }
    return true;
}

void strncpy(char *dst, const char *src, usize nbytes) {
    usize bytes_copied = 0;
    const char *cursor = src;
    while (*src) {
        if (bytes_copied == nbytes) {
            dst[bytes_copied-1] = '\x00';
            return;
        }
        *dst = *src;
        dst++;
        src++;
        bytes_copied++;
    }
    *dst = '\x00';
}

usize strlen(const char *s) {
    const char *cursor = s;
    usize len = 0;
    while (*s) {
        s++;
        len++;
    }
    return len;
}

// Equal == true, not equal == false
bool strequal(char *a, char *b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++;
        b++;
    }
    return *a == *b;
}

/*
 * init_sbrk_heap
 * Tells lib/mem.c where to mark the sbrk heap
 * used by dlmalloc. This must be called early
 * during initialization prior to using dlmalloc
 * or anything requiring operator new.
 */
void init_sbrk_heap(usize base, usize len) {
    sbrk_heap_base = base;
    sbrk_heap_len = len;
    heap_ptr = (u8 *)base;
}

void *sbrk(int incr) {
    if (!heap_ptr) {
        // Initialize heap
        panic("Null heap");
    }

    if (((usize)heap_ptr + incr) > sbrk_heap_base + sbrk_heap_len) {
        panic("Heap exhausted");
        return (void *)-1;
    }

    heap_ptr += incr;
    return heap_ptr;
}

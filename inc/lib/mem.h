#ifndef MEM_H
#define MEM_H

#include <fractal.h>

BEGIN_C_HEADER

#define PAGE_HEAP_SIZE ((256 * ONE_MB))
#define SBRK_HEAP_SIZE ((256 * ONE_MB))

void *memset(void *b, int c, size_t len);
void *memcpy(void *dst, const void *src, size_t n);
void strncpy(char *dst, const char *src, usize nbytes);
usize strlen(const char *s);

// Return true if equal, false otherwise
bool memcmp(const void *a, const void *b, usize s);

// strequal- returns true if equal, false otherwise, as it should be
bool strequal(char *a, char *b);

extern u8 _init_array_begin;
extern u8 _init_array_end;

// Have we setup enough of the page tables / CPU state
// to support fast memset and memcpy, which may use misaligned stores?
extern bool __fast_memops_active;

/*
 * init_sbrk_heap
 * Tells lib/mem.c where to mark the sbrk heap
 * used by dlmalloc. This must be called early
 * during initialization prior to using dlmalloc
 * or anything requiring operator new.
 */
void init_sbrk_heap(usize base, usize len);

END_C_HEADER

#endif // MEM_H

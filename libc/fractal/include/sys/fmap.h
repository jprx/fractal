#ifndef FRACTAL_MMAN_H
#define FRACTAL_MMAN_H

#include <stdint.h>

// POSIX compatibility with mmap / mprotect
// NOTE: these flags are currently unsupported and ignored by the kernel
#define PROT_NONE  0x00
#define PROT_READ  0x01
#define PROT_WRITE 0x02
#define PROT_EXEC  0x04
#define PROT_ALL ((PROT_READ | PROT_WRITE | PROT_EXEC))

// Non-standard protection flags for Fractal programs
#define PROT_USER    0x10
#define PROT_KERNEL  0x20

int mprotect(void *, size_t, uint64_t);

// Attributes for fthread_create
#define FTHREAD_USER_THREAD   ((PROT_USER))
#define FTHREAD_KERNEL_THREAD ((PROT_KERNEL))
#define FTHREAD_PREEMPTIVE  ((0x40))
#define FTHREAD_COOPERATIVE ((0x80))

// Allocate a page from the kernel page heap
// n_copies: how many VAs should map to the same PA?
// log_stride: if n_copies > 1, how far apart in the VA space should the pages be placed?
int fmap_strided(void **newpgptr, size_t n_copies, size_t log_stride);

// fmap but with n_copies = 1 and log_stride = 0 (defaults)
int fmap(void **newpgptr);

// free something allocated with fmap / fmap_strided
int funmap(void *pgptr);

// returns 2^12 giant pages worth of virtual addresses, all mapping to the same 2MB page
int gmap_alloc(void **newpgptr);

// carves out a 2MB page from the gigamap to point to a new physical page
int gmap_carve(void *carveout);

// returns a pointer to a 2MB virtual address that maps to the underlying default PA in the gigamap
int gmap_get_kbase(void **baseptr);
int gmap_get_ubase(void **baseptr);

// reset the entire gigamap so that every VA points to the default PA
// causes any carved out pages + their internal PTE pages to be freed
int gmap_reset();

// frees and unmaps the entire gigamap
int gmap_unmap();

// Translate a virtual addr to its physical addr
int xlate_v2p(uint64_t va, uint64_t *pa_ptr);

#endif // FRACTAL_MMAN_H

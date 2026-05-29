#ifndef FRACTAL_ULTRATHREAD_H
#define FRACTAL_ULTRATHREAD_H

#include <stdint.h>

// "Ultrathread mode" is a special scheduler mode where Fractal alternates
// rapidly between two threads, one in userspace and one in kernelspace.
// These "ultra context switches" are super short, minimizing
// microarchitectural impact, and crucially happen without ANY branches.
// Therefore, the branch predictor will be fully untouched between thread
// switches. The TLB and caches will be minimally affected as well.

#if !defined(__aarch64__)
#error "Ultrathreads are only supported on ARM"
#endif // FRACTAL_ARM

typedef struct {
    uint64_t x[31];
    uint64_t sp_el0;
    uint64_t spsr;
    uint64_t elr;
} ultra_regs_t;

#define ultrathread_switch() ({ asm volatile("svc #0"); })

void ultrathread_begin(void __attribute__((noreturn)) (*)(void));
void ultrathread_end();

#endif // FRACTAL_ULTRATHREAD_H

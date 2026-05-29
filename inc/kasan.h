#ifndef KASAN_H
#define KASAN_H

#include <fractal.h>

BEGIN_C_HEADER

#ifdef KASAN

void asan_load(void *p, usize s);
void asan_store(void *p, usize s);

#define KASAN_IGNORE __attribute__((no_sanitize_address))

#else // KASAN

#define KASAN_IGNORE

#endif // ! KASAN

typedef enum {
    KASAN_LOAD,
    KASAN_STORE
} kasan_op_t;

typedef struct {
    u64            k_ptr;
    // usize          k_size;
    // kasan_op_t     k_op;
} kasan_trace_t;

void kasan_begin_trace();
void kasan_end_trace();
void kasan_dump_trace();

END_C_HEADER

#endif // KASAN_H

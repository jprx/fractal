#include <fractal.h>
#include <page_alloc.h>
#include <kasan.h>
#include <paging.h>

// @TODO: Learn cache parameters automatically from hardware
// These are for the Apple M1 pcore CPU
#define L1D_LINESZ 64
#define L1D_NSETS 256
#define L1D_MASK ((L1D_NSETS - 1))
#define L1D_SHIFT ((6ull))

#ifdef KASAN

static bool g_kasan_on = false;
static kasan_trace_t *g_tracebuf = NULL;
static usize g_trace_idx = 0;
static usize g_trace_end = 0;

static inline KASAN_IGNORE bool is_out_of_bounds(usize i) {
    return ((i+1) * sizeof(g_tracebuf[0])) > LARGE_PAGE_SIZE;
}

KASAN_IGNORE void asan_record_op(void *p, kasan_op_t kind) {
    if (!g_kasan_on) return;

    if (is_out_of_bounds(g_trace_idx)) {
        g_kasan_on = false;
        panic("kasan: trace buffer ran out of memory");
    }
    g_tracebuf[g_trace_idx].k_ptr = (u64)p;
    g_trace_idx++;
}

KASAN_IGNORE void asan_load(void *p, usize s) {
    asan_record_op(p, KASAN_LOAD);
}

KASAN_IGNORE void asan_store(void *p, usize s) {
    asan_record_op(p, KASAN_STORE);
}

KASAN_IGNORE void kasan_begin_trace() {
    if (!g_tracebuf) g_tracebuf = (kasan_trace_t *)AllocPage(LARGE_PAGE);
    if (!g_tracebuf) panic("kasan: failed to create the trace buffer");
    g_trace_idx = 0;
    g_kasan_on = true;
}

KASAN_IGNORE void kasan_end_trace() {
    g_kasan_on = false;
    g_trace_end = g_trace_idx;
}

KASAN_IGNORE void kasan_dump_trace() {
    u64 cache_sets_touched[L1D_NSETS]; // Histogram for cache utilization during this trace
    if (0 == g_trace_end) return;

    memset(cache_sets_touched, '\x00', sizeof(cache_sets_touched));
    printf("==== begin kasan dump ====\n");
    printf("g_tracebuf: 0x%X\n", g_tracebuf);
    printf("g_trace_end: %d\n", g_trace_end);
    printf("---- raw addresses ----\n");
    for (usize i = 0; i < g_trace_end; i++) {
        cache_sets_touched[((g_tracebuf[i].k_ptr) >> L1D_SHIFT) & (L1D_MASK)]++;
        printf("\t0x%X : 0x%X\n", g_tracebuf[i].k_ptr, KERN_V2P(g_tracebuf[i].k_ptr));
    }
    printf("---- cache sets used ----\n");
    for (usize i = 0; i < L1D_NSETS; i++) {
        if (0 != cache_sets_touched[i]) {
            printf("%d: %d accesses\n", i, cache_sets_touched[i]);
        }
    }
    printf("==== end kasan dump ====\n");
}

KASAN_IGNORE void __asan_load1(void *p) { asan_load(p, 1); }
KASAN_IGNORE void __asan_load2(void *p) { asan_load(p, 2); }
KASAN_IGNORE void __asan_load4(void *p) { asan_load(p, 4); }
KASAN_IGNORE void __asan_load8(void *p) { asan_load(p, 8); }
KASAN_IGNORE void __asan_load16(void *p) { asan_load(p, 16); }
KASAN_IGNORE void __asan_load1_noabort(void *p) { asan_load(p, 1); }
KASAN_IGNORE void __asan_load2_noabort(void *p) { asan_load(p, 2); }
KASAN_IGNORE void __asan_load4_noabort(void *p) { asan_load(p, 4); }
KASAN_IGNORE void __asan_load8_noabort(void *p) { asan_load(p, 8); }
KASAN_IGNORE void __asan_load16_noabort(void *p) { asan_load(p, 16); }
KASAN_IGNORE void __asan_loadN_noabort(void *p) { asan_load(p, -1); }
KASAN_IGNORE void __asan_store1(void *p) { asan_store(p, 1); }
KASAN_IGNORE void __asan_store2(void *p) { asan_store(p, 2); }
KASAN_IGNORE void __asan_store4(void *p) { asan_store(p, 4); }
KASAN_IGNORE void __asan_store8(void *p) { asan_store(p, 8); }
KASAN_IGNORE void __asan_store16(void *p) { asan_store(p, 16); }
KASAN_IGNORE void __asan_store1_noabort(void *p) { asan_store(p, 1); }
KASAN_IGNORE void __asan_store2_noabort(void *p) { asan_store(p, 2); }
KASAN_IGNORE void __asan_store4_noabort(void *p) { asan_store(p, 4); }
KASAN_IGNORE void __asan_store8_noabort(void *p) { asan_store(p, 8); }
KASAN_IGNORE void __asan_store16_noabort(void *p) { asan_store(p, 16); }
KASAN_IGNORE void __asan_storeN_noabort(void *p) { asan_store(p, -1); }

// Unused:
KASAN_IGNORE void __asan_before_dynamic_init() {}
KASAN_IGNORE void __asan_after_dynamic_init() {}
KASAN_IGNORE void __asan_handle_no_return() {}

#else // KASAN

// When not building KASAN, just stub out our public APIs
void kasan_begin_trace() {}
void kasan_end_trace() {}
void kasan_dump_trace() {}

#endif // ! KASAN

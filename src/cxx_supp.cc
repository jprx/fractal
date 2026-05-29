#include <lib/mem.h>
#include <lib/dlmalloc.h>

typedef void (*ctor_fn_t)(void);

extern "C" void __cxa_pure_virtual() {}

/*
 * GCC gives us a section .init_array that contains function pointers we must
 * run prior to using any global C++ objects.
 */
extern "C" void run_ctors() {
    for (usize entry = (usize)&_init_array_begin;
            entry < (usize)&_init_array_end;
            entry+=sizeof(usize)) {
        ((ctor_fn_t)(*(usize **)entry))();
    }
}

void *operator new(usize c) {
    return dlmalloc(c);
}

void operator delete(void *p, usize l) {
    dlfree(p);
}

void *__dso_handle = NULL;
extern "C" int __cxa_atexit(void (*destructor)(void *), void *arg, void *dso_handle) {
    return 0;
}
extern "C" void __cxa_finalize(void *f) {

}

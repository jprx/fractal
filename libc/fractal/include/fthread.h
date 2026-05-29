#ifndef FTHREAD_H
#define FTHREAD_H

#include <stdint.h>
#include <sys/fmap.h>

// Userspace fthreads API
typedef uint64_t fthread_t;
int fthread_create(fthread_t *f, uint64_t attr, void *(*start_fn)(void *), void *arg);
int fthread_join(fthread_t f, void **val_ptr);
int fthread_switch(fthread_t f);
int fthread_exit(void *val_ptr);

#endif // FTHREAD_H

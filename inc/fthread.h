#ifndef FTHREAD_H
#define FTHREAD_H

#include <fractal.h>

BEGIN_C_HEADER

#define FTHREAD_USER_MAP ((0x000000000000ull))
#define FTHREAD_KERN_MAP ((0x5FFF00000000ull))

#define IS_PTR_IN_FTHREAD_USER_MAP(v) (((v < (FTHREAD_KERN_MAP - 1))))
#define IS_PTR_IN_FTHREAD_KERN_MAP(v) (((v > (FTHREAD_KERN_MAP)) && (v < (1ull << 48ull))))

#define TO_FTHREAD_USER_MAP(v) ((((v) & (~FTHREAD_KERN_MAP))))
#define TO_FTHREAD_KERN_MAP(v) ((((v) | (FTHREAD_KERN_MAP))))

/*
 * fthread_exception
 * Attempt to fixup an exception caused by fthreads running in different privilege levels.
 * Eg. an outer kernel thread tried to use a user function pointer.
 *
 * Returns true if we cleaned up the exception, false otherwise.
 */
bool fthread_exception(void *regs_in);

END_C_HEADER

#endif // FTHREAD_H

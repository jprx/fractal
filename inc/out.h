#ifndef OUT_H
#define OUT_H

#include <types.h>
#include <stdarg.h>

BEGIN_C_HEADER
kret_t printf(char *format, ...);
usize snprintf(char *outs, char *format, ...);
void __global_out_putc(char c);
void __global_out_puts(char *s);

/*
 * panic
 * Unrecoverable error- call this if there
 * is no other possible option.
 */
__attribute__((noreturn)) void panic(char *reason, ...);

END_C_HEADER

#endif // OUT_H

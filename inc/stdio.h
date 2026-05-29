#ifndef FRACTAL_STDIO_H
#define FRACTAL_STDIO_H

#include <fractal.h>
#include <task.h>

BEGIN_C_HEADER

// stdin, stdout, and stderr
#define NUM_STDIO_FDS ((3))

void init_serial_stdio(task_t *t);
// c is a Console C++ object
void init_console_stdio(task_t *t, void *c);
usize task_read_stdio(task_t *t, u32 fd_num, u8 *buf, usize sz);
void task_write_stdio(task_t *t, u32 fd_num, u8 *buf, usize sz);

END_C_HEADER

#endif // FRACTAL_STDIO_H

#ifndef PID_H
#define PID_H

#include <fractal.h>

BEGIN_C_HEADER

typedef u64 pid_t; // process ID
typedef u64 tid_t; // thread ID

/*
 * alloc_pid
 * Returns a new, guaranteed unique, process ID.
 */
pid_t alloc_pid();

// A special PID that means "match any process",
// for functions that are maybe looking for something by PID.
#define PID_ANY ((0xFFFFFFFFFFFFFFFFull))

END_C_HEADER

#endif // PID_H

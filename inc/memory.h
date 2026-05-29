#ifndef MEMORY_H
#define MEMORY_H

#include <fractal.h>

BEGIN_C_HEADER

// The various kinds of memory available on Fractal-supported systems

typedef enum memory_kind_t {
    MEMORY_WRITEBACK,      // "Normal"- And the default
    MEMORY_UNCACHEABLE,    // For I/O
    MEMORY_WRITECOMB,      // Write combining- for framebuffers / display devices
} memory_kind_t;

END_C_HEADER

#endif // MEMORY_H

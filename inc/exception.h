#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <fractal.h>
#include <regs.h>

BEGIN_C_HEADER

#define PROCESS_KILL_MESSAGE "Segmentation Fault\r\n"

typedef enum {
    EXC_UNKNOWN,
    EXC_PAGE_FAULT,
    EXC_ILLEGAL_INSTRUCTION,
} exception_kind;

// Every CPU sends all exceptions here:
// For EXC_PAGE_FAULT, reason == the faulting address
void Exception(regs_t *r, exception_kind kind, u64 reason);

END_C_HEADER

#endif // EXCEPTION_H
#ifdef FRACTAL_X86
#include "crt0_x86.S"
#endif // FRACTAL_X86

#ifdef FRACTAL_ARM
#include "crt0_arm.S"
#endif // FRACTAL_ARM

#ifdef FRACTAL_RISCV
#include "crt0_rv.S"
#endif // FRACTAL_RISCV

#ifndef FRACTAL_H
#define FRACTAL_H

#include <types.h>
#include <virt_mem.h>
#include <page.h>
#include <page_alloc.h>
#include <lib/mem.h>
#include <out.h>

BEGIN_C_HEADER

// Uncomment this to enable "single program mode",
// where we run a single program and then reboot the SoC.
// This is primarily useful when testing stuff on Apple SoCs.
#define SINGLE_PROGRAM_MODE

// Uncomment this to disable most boot messages
#define QUIET_BOOT

// Uncomment this to enable Oxide, the window manager / GUI
// #define USE_GRAPHICS

#ifdef ARCH_X86
#define ARCH_STRING "x86_64"
#define ARCH_HAS_FAST_MEMCPY
#define ARCH_HAS_FAST_MEMSET
#endif // ARCH_X86

#ifdef ARCH_ARM
#define ARCH_STRING "aarch64"
#define ARCH_HAS_FAST_MEMCPY
#define ARCH_HAS_FAST_MEMSET
#endif // ARCH_ARM

#ifdef ARCH_RISCV
#define ARCH_STRING "riscv64"
#define ARCH_HAS_FAST_MEMCPY
#define ARCH_HAS_FAST_MEMSET
#endif // ARCH_RISCV

// Various kernel configuration related defines
#define KERNEL_NAME "fractal"
#define HEAP_IMPLEMENTATION_NAME "dlmalloc"
#define WINDOW_MANAGER_NAME "oxide"

#ifdef VARIANT_RELEASE
#define KERNEL_VARIANT "release"
#endif // VARIANT_RELEASE

#ifdef VARIANT_DEBUG
#define KERNEL_VARIANT "debug"
#endif // VARIANT_DEBUG

#ifdef VARIANT_KASAN
#define KERNEL_VARIANT "kasan"
#endif // VARIANT_KASAN

// HVF_OPTIMIZATION_HACK is used to guard goofy/ weird
// things that need to be added to the code to make it work in Qemu HVF
// with VARIANT=release. For example, all virtio initialization
// functions must be compiled with pragma GCC push_options optimize O0.
// Only do this for ARM release builds though.
#ifdef VARIANT_RELEASE
#ifdef ARCH_ARM
#define HVF_OPTIMIZATION_HACK
#endif // ARCH_ARM
#endif // VARIANT_RELEASE

// The kernel boot logo printed to serial
extern char *logo;

#define ceil_div(a, b) ((((((a)) + ((b)) - 1))/((b))))
#define floor_div(a, b) ((((a))/((b))))
#define min(a,b) (((((a) < (b)) ? (a) : (b))))
#define max(a,b) (((((a) > (b)) ? (a) : (b))))

END_C_HEADER

#endif // FRACTAL_H

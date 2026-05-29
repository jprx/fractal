#ifndef BOARD_H
#define BOARD_H

#include <fractal.h>

// Re-define various constants used by the kernel based on what board we are being compiled for
#if ARCH_X86
#include <board/x86/generic.h>
#endif // ARCH_X86

#if ARCH_ARM

    #if BOARD_QEMU
    #include <board/arm/qemu.h>
    #endif // BOARD_QEMU

    #if BOARD_RASPI
    #include <board/arm/raspi4b.h>
    #endif // BOARD_RASPI

    #if BOARD_APPLE
    #include <board/arm/apple.h>
    #endif // BOARD_APPLE

#endif // ARCH_ARM

#if ARCH_RISCV
#include <board/rv/qemu.h>
#endif // ARCH_RISCV

// Export the board name string
#ifdef BOARD_QEMU
#define BOARD_NAME "qemu"
#endif // BOARD_QEMU

#ifdef BOARD_RASPI
#define BOARD_NAME "raspi"
#endif // BOARD_RASPI

#ifdef BOARD_APPLE
#include <board/arm/apple/soc.h>
#endif // BOARD_APPLE

#ifndef BOARD_NAME
#define BOARD_NAME "unknown"
#endif // !defined(BOARD_NAME)

#endif // BOARD_H

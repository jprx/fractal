#ifndef APPLE_ARM_SOC_H
#define APPLE_ARM_SOC_H

// Every supported Apple Silicon device needs the address of
// the serial port and watchdog timer hardcoded here.
// TODO: Parse the device tree to learn this info instead.

#if !defined(BOARD_APPLE)
#error "Compiling apple/soc.h for a non-Apple board"
#endif

#if !defined(SOC_M1) && !defined(SOC_M4) && !defined(SOC_VMAPPLE)
#error "When compiling for an Apple platform, you need to specify SOC=$(KIND)"
#endif

// M1 Mac Mini:
#ifdef SOC_M1
#define APPLE_SOC_MODE_REALHW
#define APPLE_SERIAL_BASEADDR 0x235200000ull
#define APPLE_WDT_BASEADDR    0x23D2B0000ull
#define BOARD_NAME "apple-m1"
#endif // SOC_M1

// M4 Mac Mini:
#ifdef SOC_M4
#define APPLE_SOC_MODE_REALHW
#define APPLE_SERIAL_BASEADDR 0x3AD200000ull
#define APPLE_WDT_BASEADDR    0x3882B0000ull
#define BOARD_NAME "apple-m4"
#endif // SOC_M4

// VMAPPLE vma2:
#ifdef SOC_VMAPPLE
#define APPLE_SOC_MODE_VIRT
#define APPLE_SERIAL_BASEADDR 0x20010000ull
#define BOARD_NAME "apple-vmapple"
#endif // SOC_VMAPPLE

#ifdef APPLE_SOC_MODE_REALHW
// Apple UART
#define APPLE_SERIAL_WRITE_PORT_OFFSET 0x20ull
#endif // APPLE_SOC_MODE_REALHW

#ifdef APPLE_SOC_MODE_VIRT
// Really a PL011
#define APPLE_SERIAL_WRITE_PORT_OFFSET 0x00ull
#endif // APPLE_SOC_MODE_VIRT

#endif // APPLE_ARM_SOC_H
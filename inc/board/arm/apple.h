#ifndef BOARD_APPLESI_H
#define BOARD_APPLESI_H

#include <fractal.h>
#include <board/arm/apple/soc.h>

BEGIN_C_HEADER

/*
 * Definitions for running Fractal on an Apple Silicon based computer.
 */

#ifndef ARCH_ARM
#error "Including apple.h for non-ARM target"
#endif // !ARCH_ARM

#include <io/serial/apple_uart.h>
#include <io/serial/pl011.h>
#include <virt_mem.h>
#include <io/pci.h>

// Apple M4:
#ifdef APPLE_SOC_MODE_REALHW
#define SERIAL_PORT AppleUART
#define SERIAL_BASEADDR KERN_P2V(0x3AD200000)
#endif // APPLE_SOC_MODE_REALHW

// VMAPPLE (Virtualization.framework VMs):
#ifdef APPLE_SOC_MODE_VIRT
#define SERIAL_PORT PL011
#define SERIAL_BASEADDR KERN_P2V(0x20010000)
#endif // APPLE_SOC_MODE_VIRT

// Unused on Apple Silicon:
#define ARM_INITRD_BASE KERN_P2V(0x0000)
#define UART0_IRQ ((0))
#define PLATFORM_PCI_MMCFG_BASE PLATFORM_PCI_MMCFG_BASE_NULL

#define BOARD_HAS_CUSTOM_INITRD_FINDER
#define BOARD_NO_1GB_PAGES
#define BOARD_HAS_BRINGUP_FUNCTION
#define BOARD_HAS_REBOOT

static inline u64 *__board_find_initrd__() {
    extern u64 _apple_initrd_base;
    return (u64 *)(&_apple_initrd_base);
}

// Detected at runtime from the ADT:
extern u64 apple_uart_base_phys;

END_C_HEADER

#endif // BOARD_APPLESI_H

#ifndef BOARD_RASPI4B_H
#define BOARD_RASPI4B_H

#include <fractal.h>

BEGIN_C_HEADER

/*
 * Definitions for running Fractal on a Raspberry Pi 4B.
 */

#ifndef ARCH_ARM
#error "Including raspi4b.h for non-ARM target"
#endif // !ARCH_ARM

#include <io/serial/raspi_uart.h>
#include <virt_mem.h>
#include <io/pci.h>

#define ARM_BOARD_HAS_GIC

#define SERIAL_PORT RaspiUART4B

// The serial base address is unused by the Raspi UART driver, as we instead
// hardcode the entire MMIO region in the raspi drivers.
#define SERIAL_BASEADDR ((0))

#define PLATFORM_PCI_MMCFG_BASE PLATFORM_PCI_MMCFG_BASE_NULL

// This is where Qemu puts my initrd for -M raspi4b.
// On real hardware we can setup config.txt to place our initrd here for us.
#define ARM_INITRD_BASE KERN_P2V((0x0000000008000000))

#define RPI_MMIO_BASE KERN_P2DV(0xFE000000)
#define RASPI_VERSION 4

#define GICD_DEFAULT_BASE   KERN_P2DV((0xff841000))
#define GICC_DEFAULT_BASE   KERN_P2DV((0xff842000))
#define GIC_NUM_INTERRUPTS ((256))

// All PL011 UARTs are VPU interrupt 57,
// And the VPUs are mapped starting at 96.
#define UART0_IRQ 153

END_C_HEADER

#endif // BOARD_RASPI4B_H

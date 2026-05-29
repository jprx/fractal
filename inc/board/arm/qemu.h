#ifndef BOARD_QEMU_ARM_H
#define BOARD_QEMU_ARM_H

#include <fractal.h>

BEGIN_C_HEADER

#ifndef ARCH_ARM
#error "Including board/arm/qemu.h for non-ARM target"
#endif // !ARCH_ARM

#include <io/serial/pl011.h>

#define ARM_BOARD_HAS_GIC

#define SERIAL_PORT PL011
#define SERIAL_BASEADDR ((((0x09000000)) | KERNEL_DEVMEM_VIRT_MASK))

#define PLATFORM_PCI_MMCFG_BASE ((0x000004010000000))
#define ARM_INITRD_BASE ((0xFFFF800048000000))

// qemu/hw/arm/virt.c:159
#define GICD_DEFAULT_BASE   KERN_P2DV((0x08000000))
#define GICC_DEFAULT_BASE   KERN_P2DV((0x08010000))

#define GIC_NUM_INTERRUPTS ((128))

#define UART0_IRQ 33

END_C_HEADER

#endif // BOARD_QEMU_ARM_H

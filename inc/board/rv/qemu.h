#ifndef BOARD_QEMU_RISCV_H
#define BOARD_QEMU_RISCV_H

#include <fractal.h>

BEGIN_C_HEADER

#ifndef ARCH_RISCV
#error "Including board/rv/qemu.h for non-RISCV target"
#endif // !ARCH_RISCV

#define SERIAL_PORT UART16550MMIO
#define SERIAL_BASEADDR ((((0x10000000)) | KERNEL_DEVMEM_VIRT_MASK))

#define RISCV_INITRD_BASE ((0xffff8000a0000000))
#define QEMU_RISCV_VIRT_PCI_MMCFG_BASE ((0x30000000))

END_C_HEADER

#endif // BOARD_QEMU_RISCV_H

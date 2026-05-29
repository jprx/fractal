#include <io/serial.h>
#include <io/serial/uart16550.h>

UART16550MMIO::UART16550MMIO(usize baseaddr_in) {
    this->baseaddr = baseaddr_in;
}

u8 UART16550MMIO::ReadReg(u16 reg) {
    if (reg > UART16550_NUM_REGS) return 0;
    volatile u8 *regs = (u8 *)baseaddr;
    return regs[reg];
}

void UART16550MMIO::WriteReg(u16 reg, u8 val) {
    if (reg > UART16550_NUM_REGS) return;
    volatile u8 *regs = (u8 *)baseaddr;
    regs[reg] = val;
}

#include <io/serial.h>
#include <io/serial/uart16550.h>
#include <arch.h>

UART16550Port::UART16550Port(usize baseaddr_in) {
    this->baseaddr = baseaddr_in;
}

u8 UART16550Port::ReadReg(u16 reg) {
    if (reg > UART16550_NUM_REGS) return 0;
    return inb(baseaddr + reg);
}

void UART16550Port::WriteReg(u16 reg, u8 val) {
    if (reg > UART16550_NUM_REGS) return;
    outb(baseaddr + reg, val);
}

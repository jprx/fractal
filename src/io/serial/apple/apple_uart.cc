/*
 * THIRD PARTY CODE USED IN THIS FILE.
 * This code is copied or derived from code retrieved
 * from the Asahi Linux m1n1 project on April 3, 2025.
 *
 * git@github.com:AsahiLinux/m1n1.git
 */

#include <fractal.h>
#include <io/serial.h>
#include <io/serial/apple_uart.h>
#include <board/arm/apple/soc.h>

#define ULCON    0x000
#define UCON     0x004
#define UFCON    0x008
#define UTRSTAT  0x010
#define UFSTAT   0x018
#define UTXH     0x020
#define URXH     0x024
#define UBRDIV   0x028
#define UFRACVAL 0x02c

#define UCON_TXTHRESH_ENA BIT(13)
#define UCON_RXTHRESH_ENA BIT(12)
#define UCON_RXTO_ENA     BIT(9)

#define UCON_MODE_OFF 0
#define UCON_MODE_IRQ 1

#define UTRSTAT_RXTO     BIT(9)
#define UTRSTAT_TXTHRESH BIT(5)
#define UTRSTAT_RXTHRESH BIT(4)
#define UTRSTAT_TXE      BIT(2)
#define UTRSTAT_TXBE     BIT(1)
#define UTRSTAT_RXD      BIT(0)

#define UFSTAT_TXFULL BIT(9)
#define UFSTAT_RXFULL BIT(8)

#define UART_BASE ((KERN_P2V((APPLE_SERIAL_BASEADDR))))

// Borrowed from m1n1 (https://github.com/AsahiLinux/m1n1/blob/main/src/utils.h#L85)
static inline u32 read32(u64 addr)
{
    u32 data;
    asm volatile("ldr %w0, [%1]" : "=r"(data) : "r"(addr) : "memory");
    return data;
}

// Borrowed from m1n1 (https://github.com/AsahiLinux/m1n1/blob/main/src/utils.h#L92)
static inline void write32(u64 addr, u32 data)
{
    asm volatile("str %w0, [%1]" : : "r"(data), "r"(addr) : "memory");
}

AppleUART::AppleUART(usize baseaddr_in) {
    this->baseaddr = baseaddr_in;
}

void AppleUART::Initialize() {
    write32(UART_BASE + UFCON, 0); // disable FIFO mode

    // enable (level-triggered) rx interrupts- NOT WORKING for interrupts
    // but this is required to make the UART give us 1 char at a time for some reason.
    write32(UART_BASE + UCON, read32(UART_BASE + UCON) | BIT(0) | BIT(6) | BIT(8));
}

void AppleUART::SendChar(u8 c) {
    while (!(read32(UART_BASE + UTRSTAT) & UTRSTAT_TXBE));
    write32(UART_BASE + UTXH, c);
}

u8 AppleUART::RecvChar() {
    while (!(read32(UART_BASE + UTRSTAT) & UTRSTAT_RXD));
    return read32(UART_BASE + URXH);
}

bool AppleUART::HasData() {
    return !(!(read32(UART_BASE + UTRSTAT) & UTRSTAT_RXD));
}

#define WDT_COUNT 0x10
#define WDT_ALARM 0x14
#define WDT_CTL   0x1c

extern "C" void __board_bringup__() {
    write32(APPLE_WDT_BASEADDR + WDT_CTL, 0);
}

extern "C" void __board_reboot__() {
    // m1n1/src/wdt.c
    write32(KERN_P2V(APPLE_WDT_BASEADDR + WDT_ALARM), 0x100000);
    write32(KERN_P2V(APPLE_WDT_BASEADDR + WDT_COUNT), 0);
    write32(KERN_P2V(APPLE_WDT_BASEADDR + WDT_CTL), 4);
}
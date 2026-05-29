/*
 * THIRD PARTY CODE USED IN THIS FILE.
 * This code is copied or derived from code retrieved
 * from the OS Dev Wiki on December 19, 2024.
 *
 * https://wiki.osdev.org/Raspberry_Pi_Bare_Bones
 */

#ifndef RASPI_UART_DELAY_H
#define RASPI_UART_DELAY_H

// Syntax highlighting breaks because of this function, so throw it in its own header:

// Loop <delay> times in a way that the compiler won't optimize away
static inline void delay(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : "=r"(count): [count]"0"(count) : "cc");
}

#endif // RASPI_UART_DELAY_H

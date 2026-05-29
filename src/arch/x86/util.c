#include <types.h>

#if ARCH_X86

void outb(u16 port, u8 val) {
	asm volatile(
		"outb %0, %1"
		:
		: "dN"(port), "a"(val)
	);
}

void outw(u16 port, u16 val) {
	asm volatile(
		"outw %0, %1"
		:
		: "dN"(port), "ax"(val)
	);
}

u8 inb(u16 port) {
	u8 outval;
	asm volatile(
		"inb %0, %1"
		: "=a"(outval)
		: "dN"(port)
	);
	return outval;
}

u64 read_msr (u32 msr_idx) {
	u32 val_hi;
	u32 val_low;
	asm volatile(
		"rdmsr"
		: "=a"(val_low), "=d"(val_hi)
		: "c"(msr_idx)
	);

	return ((u64)val_hi << 32) | val_low;
}

void write_msr(u32 msr_idx, u64 val) {
	u32 val_hi = (val >> 32);
	u32 val_low = val;

	asm volatile(
		"wrmsr"
		:: "c"(msr_idx), "a"(val_low), "d"(val_hi)
	);
}

#endif // ARCH_X86

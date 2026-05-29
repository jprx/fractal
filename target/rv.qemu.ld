ENTRY(fractal_start);

QEMU_RISCV_LOAD_BASE = 0x80000000;

KERNEL_LINK_ADDRESS = 0xFFFF800000000000;

INITRD_ADDRESS = 0xFFFF8000A0000000;

PAGE_SIZE = 4K;
HUGE_PAGE_SIZE = 2M;
ONE_GB = 1024M;

SECTIONS {
	. = QEMU_RISCV_LOAD_BASE;
	PROVIDE(__very_start_of_kernel_mem = .);
	.text.start ALIGN(PAGE_SIZE) : {*(.text.start);*(.text.start_c);}

	. += KERNEL_LINK_ADDRESS;
	.text ALIGN(PAGE_SIZE) : AT(ADDR(.text) - KERNEL_LINK_ADDRESS) {
		PROVIDE(_fractal_start64_lma = . - KERNEL_LINK_ADDRESS);
		*(.text.start64);
		*(.text);
		*(.text.*);
	}
	.rodata ALIGN(PAGE_SIZE) : AT(ADDR(.rodata) - KERNEL_LINK_ADDRESS) {*(.rodata); *(.rodata.*);}
	.bss ALIGN(PAGE_SIZE) : AT(ADDR(.bss) - KERNEL_LINK_ADDRESS) {*(.bss); *(.bss.*);}
	.data ALIGN(PAGE_SIZE) : AT(ADDR(.data) - KERNEL_LINK_ADDRESS) {*(.data); *(.data.*);}

	/* This is an array of function pointers we have to run before using any global C++ objects */
	.init_array ALIGN(PAGE_SIZE) : AT(ADDR(.init_array) - KERNEL_LINK_ADDRESS) {
		PROVIDE(_init_array_begin = .);
		*(.init_array);
		PROVIDE(_init_array_end = .);
	}
	/* Basic stack for bringup tasks, after setting up paging */
	.stack : {
		PROVIDE(_stack_top_lma = .);
		. += 1M;
		PROVIDE(_stack_bottom_lma = .);
	}

	PROVIDE(_last_kernel_address = .);
}

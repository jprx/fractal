ENTRY(fractal_start);

RPI_LOAD_BASE = 0x80000;

KERNEL_LINK_ADDRESS = 0xFFFF800000000000;

PAGE_ALIGNMENT = 4096;
PAGE_SIZE = 4K;
ONE_GB = 1024M;
HUGE_PAGE_SIZE = 2M;
LOAD_BASE = RPI_LOAD_BASE;

SECTIONS {
	. = LOAD_BASE;
	PROVIDE(__very_start_of_kernel_mem = .);

	/* Lower-Half stuff (before paging is enabled) */
	.text.start ALIGN(PAGE_SIZE) : {
		*(.text.start);
		*(.text.start_c);
	}

	/* Higher-Half stuff (proper C/C++ land) */
	. += KERNEL_LINK_ADDRESS;
	.text ALIGN(PAGE_SIZE) : AT(ADDR(.text) - KERNEL_LINK_ADDRESS) {
		PROVIDE(_fractal_start64_lma = . - KERNEL_LINK_ADDRESS);
		*(.text.start64);
		*(.text);
		*(.text.*);
		*(.got.plt);
	}

	.rodata ALIGN(PAGE_SIZE) : AT(ADDR(.rodata) - KERNEL_LINK_ADDRESS) {
		*(.rodata);
		*(.rodata.*);
	}

	.data ALIGN(PAGE_SIZE) : AT(ADDR(.data) - KERNEL_LINK_ADDRESS) {
		*(.data);
		*(.data.*);
		. = ALIGN(8);
		*(.got);
	}

	/* This is an array of function pointers we have to run before using any global C++ objects */
	.init_array ALIGN(PAGE_SIZE) : AT(ADDR(.init_array) - KERNEL_LINK_ADDRESS) {
		PROVIDE(_init_array_begin = .);
		*(.init_array);
		PROVIDE(_init_array_end = .);
	}

	/* Basic stack for bringup tasks, after setting up paging */
	.stack ALIGN(PAGE_SIZE) : {
		PROVIDE(_stack_top_lma = .);
		. += 1M;
		PROVIDE(_stack_bottom_lma = .);

		/* This should NOT be in the higher half */
		PROVIDE(_bringup_stack_bottom = . - KERNEL_LINK_ADDRESS);
	}

	.bss ALIGN(PAGE_SIZE) : AT(ADDR(.bss) - KERNEL_LINK_ADDRESS) {
		PROVIDE(_bss_start = . - KERNEL_LINK_ADDRESS);
		*(.bss);
		*(.bss.*);
		PROVIDE(_bss_end = . - KERNEL_LINK_ADDRESS);
	}

	PROVIDE(_last_kernel_address = .);
}
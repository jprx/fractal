ENTRY(fractal_start);

/* qemu's -kernel option only supports multiboot 1, so that's what we will support for qemu targets */

MULTIBOOT_MAGIC = 0x1BADB002; /* Multiboot 1 */
MULTIBOOT_FLAGS = 4; /* flags[16] set means address fields valid. Setting it to zero just lets the bootloader handle it */
MULTIBOOT_CHECKSUM = ((-(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)));

/* Higher-Half kernel virtual base address */
/* 3GB */
/* KERNEL_LINK_ADDRESS = 0xC0000000; */
/* 64-Bit higher half */
KERNEL_LINK_ADDRESS = 0xFFFF800000000000;

INITRD_ADDRESS = 0xFFFF800003D14000;

/* Where the kernel is placed in physical memory */
KERNEL_LOAD_ADDRESS = 1M;

PAGE_SIZE = 0x1000;
HUGE_PAGE_SIZE = 2M;
ONE_GB = 1024M;

/* How much memory do we set aside for allocating full pages? */
PFN_ALLOCATOR_SIZE = 256M;

/* How much memory do we set aside for the general-purpose heap? */
HEAP_SIZE = 256M;

/* VESA VBE Framebuffer request (for real hardware) */
GRAPHICS_MODE_TYPE = 0;
GRAPHICS_WIDTH     = 1920;
GRAPHICS_HEIGHT    = 1080;
GRAPHICS_DEPTH     = 32;

SECTIONS {
	/* Encode multiboot info right at the beginning */
	/* This is a multiboot 1 header */
	. = KERNEL_LOAD_ADDRESS;
	PROVIDE(__very_start_of_kernel_mem = .);
	.multiboot_header : ALIGN(4) {
		_header_start = .;
		LONG(MULTIBOOT_MAGIC);
		LONG(MULTIBOOT_FLAGS);
		LONG(MULTIBOOT_CHECKSUM);

		/* Header Address Fields (unused) */
		/* Set flags[16] to request these */
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(0);

		/* Header Graphics Fields (unused for Virtio-GPU Qemu targets) */
		/* Set flags[2] to request these */
		LONG(GRAPHICS_MODE_TYPE);
		LONG(GRAPHICS_WIDTH);
		LONG(GRAPHICS_HEIGHT);
		LONG(GRAPHICS_DEPTH);
	}

	/* .text.start switches from protected mode -> long mode and sets up paging */
	. += PAGE_SIZE;
	.text.start : ALIGN(PAGE_SIZE) {*(.text.start);}

	. += KERNEL_LINK_ADDRESS;
	PROVIDE(_fractal_start64_lma = . - KERNEL_LINK_ADDRESS);
	.text.start64 ALIGN(PAGE_SIZE) : AT(ADDR(.text.start64) - KERNEL_LINK_ADDRESS) {*(.text.start64);}
	.text ALIGN(PAGE_SIZE) : AT(ADDR(.text) - KERNEL_LINK_ADDRESS) {*(.text); *(.text.*);}
	.rodata ALIGN(PAGE_SIZE) : AT(ADDR(.rodata) - KERNEL_LINK_ADDRESS) {*(.rodata); *(.rodata.*);}
	.bss ALIGN(PAGE_SIZE) : AT(ADDR(.bss) - KERNEL_LINK_ADDRESS) {*(.bss); *(.bss.*);}
	.data ALIGN(PAGE_SIZE) : AT(ADDR(.data) - KERNEL_LINK_ADDRESS) {*(.data); *(.data.*);}

	/* This is an array of function pointers we have to run before using any global C++ objects */
	.ctors ALIGN(PAGE_SIZE) : AT(ADDR(.ctors) - KERNEL_LINK_ADDRESS) {
		PROVIDE(_init_array_begin = .);
		*(.ctors);
		PROVIDE(_init_array_end = .);
	}
	/* Basic stack for bringup tasks, after setting up paging */
	.stack : ALIGN(PAGE_SIZE) {
		PROVIDE(_stack_top_lma = .);
		. += 1M;
		PROVIDE(_stack_bottom_lma = .);
	}

	PROVIDE(_last_kernel_address = .);
}

ENTRY(fractal_start);

MULTIBOOT_MAGIC = 0xE85250D6;
MULTIBOOT_ARCH = 0; /* 0 is 32 bit (protected mode) i386 / x86 */
MULTIBOOT_CHECKSUM = ((-(MULTIBOOT_MAGIC + MULTIBOOT_ARCH + (_header_end - _header_start))));

SECTIONS {
	/* Encode multiboot info right at the beginning */
	/* This is a multiboot 2 header */
	.multiboot_header : ALIGN(4) {
		_header_start = .;
		LONG(MULTIBOOT_MAGIC);
		LONG(MULTIBOOT_ARCH);
		LONG(_header_end - _header_start);
		LONG(MULTIBOOT_CHECKSUM);

		/* Information Request tag */
		SHORT(1);
		SHORT(0);
		LONG(mbi_tag_types_end - mbi_tag_types_begin);

		mbi_tag_types_begin = .;
		/* Each u32 is the tag type we want to see in the response */
		/* This must be a multiple of 16 bytes so that the next tag is 8 byte aligned. */

		LONG(6); /* Memory Map (3.6.8) */
		LONG(3); /* Modules (3.6.6) */

		/* If you add more stuff here you need to add enough so that this is still 8
		 * byte aligned. You can just repeat info requests if you want. */

		mbi_tag_types_end = .;
		/* End Information Request tag */

		/* End tag */
		SHORT(0);
		SHORT(0);
		LONG(8);

		_header_end = .;
	}

	. = 16M;
	.text : ALIGN(0x1000) {*(.text); *(.text.*);}
	.rodata : ALIGN(0x1000) {*(.rodata); *(.rodata.*);}
	.bss : ALIGN(0x1000) {*(.bss); *(.bss.*);}
	.data : ALIGN(0x1000) {*(.data); *(.data.*);}

	/* Create bringup stack */
	. = . + 1M;
	PROVIDE(_stack_top = .);
	. += 1M;
	. = ALIGN(0x1000);
	PROVIDE(_stack_bottom = .);
}

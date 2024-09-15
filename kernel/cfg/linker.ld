ENTRY(_start)

SECTIONS
{
	. = 0xC0100000;	

	__start = .;

	.text BLOCK(4K) : ALIGN(4K)
	{
		__text_start = .;
		*(.text .text.*)
		__text_end = .;
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		__rodata_start = .;
		*(.rodata .rodata.*)
		__rodata_end = .;
	}

	.data BLOCK(4K) : ALIGN(4K)
	{
		__data_start = .;
		*(.data .data.*)	
		__data_end = .;
	}

	.bss BLOCK(4K) : ALIGN(4K)
	{
		__bss_start = .;
		*(COMMON)
		*(.bss)
		__bss_end = .;
	}

	__end = .;
}

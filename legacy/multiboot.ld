/* The bootloader will look at this image and start execution at the symbol
   designated at the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* Begin putting sections at 1 MiB, a conventional place for kernels to be
	   loaded at by the bootloader. */
	. = 0x10000;

	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
	.text BLOCK(4K) : ALIGN(4K)
    {
		*(.multiboot)
        _code = .;
        *(.text.prologue)
        *(.text)
        _code_end = .;
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        _ro_s = .;
        *(.rodata)
        _ro_e = .;
    }

    .kctor_table ALIGN(4) : {
        _start_kctor_table = .;
        *(.kctor_table)
        _stop_kctor_table = .;
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        _data_s = .;
        *(.data)
        _data_e = .;

         _bss_s = .;
        *(.bss)
        _bss_e = .;
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
    }
    _end = .;

    _bss_size = _bss_e - _bss_s;
    _kctor_table_size = _stop_kctor_table - _start_kctor_table;

	/* The compiler may produce other sections, put them in the proper place in
	   in this file, if you'd like to include them in the final kernel. */
}

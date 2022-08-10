OUTPUT_FORMAT("binary")
ENTRY(_start)
SECTIONS
{
    . = 0x10000;

    .text BLOCK(4K) : ALIGN(4K)
    {
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

    .data BLOCK(4K) : ALIGN(4K)
    {
        _data_s = .;
        *(.data)
        *(.bss)
        _data_e = .;
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        _bss_s = .;
        *(.bss)
        _bss_e = .;
    }
    _end = .;


}

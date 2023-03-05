OUTPUT_FORMAT("binary")
ENTRY(kernel)
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
        _data_e = .;
        
        _bss_s = .;
        *(.bss)
        _bss_e = .;
    }

    _end = .;


}

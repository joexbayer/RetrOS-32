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

    .symbol_table ALIGN(4) : {
        _start_symbol_table = .;
        *(.symbol_table)
        _stop_symbol_table = ALIGN(4);
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

    _symbol_table_size = _stop_symbol_table - _start_symbol_table;


}

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
    _end = .;

    _bss_size = _bss_e - _bss_s;
    _kctor_table_size = _stop_kctor_table - _start_kctor_table;
}

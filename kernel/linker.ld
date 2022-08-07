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

    _end = .;


}

OUTPUT_FORMAT("binary")
ENTRY(_start)
SECTIONS
{
    . = 0x1000000;

    .text BLOCK(2K) : ALIGN(2K)
    {
        ../bin/crt0.o(.text)
        *(.text.prologue)
        *(.text)
    }

    .rodata BLOCK(2K) : ALIGN(2K)
    {
        *(.rodata)
    }
    .data BLOCK(2K) : ALIGN(2K)
    {

        *(.data)
        *(.bss)
    }
}

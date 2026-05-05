ENTRY(_start)
OUTPUT_FORMAT("elf32-i386")

SECTIONS
{
    /* Load address: 256MB (Safe within the 512MB identity map) */
    . = 0x10000000;

    .text   : { *(.text) }
    .rodata : { *(.rodata) }
    .data   : { *(.data) }
    .bss    : { *(.bss) }
}
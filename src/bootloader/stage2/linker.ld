OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
    /* Stage 2 is typically loaded by Stage 1.
     * This address (0x7E00) is a common choice for where Stage 1 might load Stage 2
     * after the MBR (0x7C00). Adjust this address based on your Stage 1's loading mechanism.
     * Alternatively, if Stage 2 is position-independent, you might not need a fixed address here,
     * but bootloaders often rely on specific load addresses.
     */
    . = 0x7E00;

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.note.GNU-stack)
        *(.comment)
    }
}
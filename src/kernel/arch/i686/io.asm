
global x86_outb
x86_outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global x86_inb
x86_inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

; Add aliases for kernel linkage
global i686_outb
i686_outb:
    jmp x86_outb

global i686_inb
i686_inb:
    jmp x86_inb
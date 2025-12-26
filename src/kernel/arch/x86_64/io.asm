
[bits 64]

global x86_64_outb
x86_64_outb:
    mov dx, di    ; port
    mov al, sil   ; value
    out dx, al
    ret

global x86_64_inb
x86_64_inb:
    mov dx, di    ; port
    xor rax, rax
    in al, dx
    ret

global x86_64_Panic
x86_64_Panic:
    cli
    hlt

global x86_64_EnableInterrupts
x86_64_EnableInterrupts:
    sti
    ret

global x86_64_DisableInterrupts
x86_64_DisableInterrupts:
    cli
    ret

global crash_me
crash_me:
    int 0x80
    ret
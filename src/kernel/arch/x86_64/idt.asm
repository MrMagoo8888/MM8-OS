[bits 64]

global x86_64_IDT_Load
x86_64_IDT_Load:
    ; Argument (IDTDescriptor*) is in rdi
    lidt [rdi]
    ret
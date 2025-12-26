[bits 64]

extern x86_64_ISR_Handler

global isr_common_stub

isr_common_stub:
    ; 1. Save CPU state
    ; We push registers in the reverse order of the Registers struct in isr.h
    ; Struct order: r15, r14, ..., rax. So we push rax first, r15 last.
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; 2. Call C handler
    ; System V AMD64 ABI: First argument goes in RDI.
    ; We pass the pointer to the stack structure (RSP) as the argument.
    mov rdi, rsp
    call x86_64_ISR_Handler

    ; 3. Restore state
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; 4. Cleanup error code and interrupt number pushed by the stub/CPU
    add rsp, 16 

    iretq
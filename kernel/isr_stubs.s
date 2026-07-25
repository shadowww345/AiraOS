[BITS 32]

extern irq_handler_dispatch

irq_common_stub:
    pusha
    mov eax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword [esp + 36]
    call irq_handler_dispatch
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 4
    iret

%macro IRQ_STUB 2
global irq%1
irq%1:
    cli
    push dword %2
    jmp irq_common_stub
%endmacro

IRQ_STUB 0,  0
IRQ_STUB 1,  1
IRQ_STUB 2,  2
IRQ_STUB 3,  3
IRQ_STUB 4,  4
IRQ_STUB 5,  5
IRQ_STUB 6,  6
IRQ_STUB 7,  7
IRQ_STUB 8,  8
IRQ_STUB 9,  9
IRQ_STUB 10, 10
IRQ_STUB 11, 11
IRQ_STUB 12, 12
IRQ_STUB 13, 13
IRQ_STUB 14, 14
IRQ_STUB 15, 15

global idt_load
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret
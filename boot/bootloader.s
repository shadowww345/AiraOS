
MAGIC    equ 0x1BADB002
FLAGS    equ 0x00
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
    align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text
extern kernel_main
global _start

_start:
    mov esp, stack_top
    cli
    call kernel_main
.hang:
    hlt
    jmp .hang

global inb
inb:
    push ebp           
    mov ebp, esp
    
    xor eax, eax         
    mov edx, [ebp + 8]  
    in al, dx      
    
    pop ebp
    ret

global outb
outb:
    mov edx, [esp + 4]   
    mov al, [esp + 8]    
    out dx, al       
    ret
    
section .bss
align 16
stack_bottom:
    resb 16384 
stack_top:

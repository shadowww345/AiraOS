MAGIC    equ 0x1BADB002
FLAGS    equ 0x00000007
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
    align 4
    dd MAGIC
    dd FLAGS
    dd -(MAGIC + FLAGS)
    dd 0, 0, 0, 0, 0
    dd 0
    dd 1024
    dd 768
    dd 32

section .text
extern kernel_main
global _start

_start:
    mov esp, stack_top
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

global bios_read_sector

bios_read_sector:
    push bp
    mov bp, sp

    mov ah, 02h
    mov al, 1
    mov ch, [bp+8]
    mov cl, [bp+12]
    mov dh, [bp+16]
    mov dl, 0
    mov bx, [bp+20]

    int 13h
    
    pop bp
    ret
    
section .bss
align 16
stack_bottom:
    resb 16384 
stack_top:
